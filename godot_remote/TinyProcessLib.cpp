// https://gitlab.com/eidheim/tiny-process-library

#include "TinyProcessLib.hpp"

namespace TinyProcessLib {

Process::Process(const std::vector<string_type> &arguments, const string_type &path,
		std::function<void(const char *bytes, size_t n)> read_stdout,
		std::function<void(const char *bytes, size_t n)> read_stderr,
		bool open_stdin, const Config &config) noexcept
		: closed(true),
		  read_stdout(std::move(read_stdout)),
		  read_stderr(std::move(read_stderr)),
		  open_stdin(open_stdin),
		  config(config) {
	open(arguments, path);
	async_read();
}

Process::Process(const string_type &command, const string_type &path,
		std::function<void(const char *bytes, size_t n)> read_stdout,
		std::function<void(const char *bytes, size_t n)> read_stderr,
		bool open_stdin, const Config &config) noexcept
		: closed(true),
		  read_stdout(std::move(read_stdout)),
		  read_stderr(std::move(read_stderr)),
		  open_stdin(open_stdin),
		  config(config) {
	open(command, path);
	async_read();
}

Process::Process(const std::vector<string_type> &arguments, const string_type &path,
		const environment_type &environment,
		std::function<void(const char *bytes, size_t n)> read_stdout,
		std::function<void(const char *bytes, size_t n)> read_stderr,
		bool open_stdin, const Config &config) noexcept
		: closed(true),
		  read_stdout(std::move(read_stdout)),
		  read_stderr(std::move(read_stderr)),
		  open_stdin(open_stdin),
		  config(config) {
	open(arguments, path, &environment);
	async_read();
}

Process::Process(const string_type &command, const string_type &path,
		const environment_type &environment,
		std::function<void(const char *bytes, size_t n)> read_stdout,
		std::function<void(const char *bytes, size_t n)> read_stderr,
		bool open_stdin, const Config &config) noexcept
		: closed(true),
		  read_stdout(std::move(read_stdout)),
		  read_stderr(std::move(read_stderr)),
		  open_stdin(open_stdin),
		  config(config) {
	open(command, path, &environment);
	async_read();
}

Process::~Process() noexcept {
	close_fds();
}

Process::id_type Process::get_id() const noexcept {
	return data.id;
}

bool Process::write(const std::string &str) {
	return write(str.c_str(), str.size());
}

} // namespace TinyProcessLib

#if !defined(_WIN32) || defined(MSYS_PROCESS_USE_SH)

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <set>
#include <stdexcept>

namespace TinyProcessLib {

Process::Data::Data() noexcept : id(-1) {}

Process::Process(const std::function<void()> &function,
		std::function<void(const char *, size_t)> read_stdout,
		std::function<void(const char *, size_t)> read_stderr,
		bool open_stdin, const Config &config) noexcept
		: closed(true),
		  read_stdout(std::move(read_stdout)),
		  read_stderr(std::move(read_stderr)),
		  open_stdin(open_stdin),
		  config(config) {
	open(function);
	async_read();
}

Process::id_type Process::open(const std::function<void()> &function) noexcept {
	if (open_stdin)
		stdin_fd = std::unique_ptr<fd_type>(new fd_type);
	if (read_stdout)
		stdout_fd = std::unique_ptr<fd_type>(new fd_type);
	if (read_stderr)
		stderr_fd = std::unique_ptr<fd_type>(new fd_type);

	int stdin_p[2], stdout_p[2], stderr_p[2];

	if (stdin_fd && pipe(stdin_p) != 0)
		return -1;
	if (stdout_fd && pipe(stdout_p) != 0) {
		if (stdin_fd) {
			close(stdin_p[0]);
			close(stdin_p[1]);
		}
		return -1;
	}
	if (stderr_fd && pipe(stderr_p) != 0) {
		if (stdin_fd) {
			close(stdin_p[0]);
			close(stdin_p[1]);
		}
		if (stdout_fd) {
			close(stdout_p[0]);
			close(stdout_p[1]);
		}
		return -1;
	}

	id_type pid = fork();

	if (pid < 0) {
		if (stdin_fd) {
			close(stdin_p[0]);
			close(stdin_p[1]);
		}
		if (stdout_fd) {
			close(stdout_p[0]);
			close(stdout_p[1]);
		}
		if (stderr_fd) {
			close(stderr_p[0]);
			close(stderr_p[1]);
		}
		return pid;
	} else if (pid == 0) {
		if (stdin_fd)
			dup2(stdin_p[0], 0);
		if (stdout_fd)
			dup2(stdout_p[1], 1);
		if (stderr_fd)
			dup2(stderr_p[1], 2);
		if (stdin_fd) {
			close(stdin_p[0]);
			close(stdin_p[1]);
		}
		if (stdout_fd) {
			close(stdout_p[0]);
			close(stdout_p[1]);
		}
		if (stderr_fd) {
			close(stderr_p[0]);
			close(stderr_p[1]);
		}

		if (!config.inherit_file_descriptors) {
			// Optimization on some systems: using 8 * 1024 (Debian's default _SC_OPEN_MAX) as fd_max limit
			int fd_max = std::min(8192, static_cast<int>(sysconf(_SC_OPEN_MAX))); // Truncation is safe
			if (fd_max < 0)
				fd_max = 8192;
			for (int fd = 3; fd < fd_max; fd++)
				close(fd);
		}

		setpgid(0, 0);
		//TODO: See here on how to emulate tty for colors: http://stackoverflow.com/questions/1401002/trick-an-application-into-thinking-its-stdin-is-interactive-not-a-pipe
		//TODO: One solution is: echo "command;exit"|script -q /dev/null

		if (function)
			function();

		_exit(EXIT_FAILURE);
	}

	if (stdin_fd)
		close(stdin_p[0]);
	if (stdout_fd)
		close(stdout_p[1]);
	if (stderr_fd)
		close(stderr_p[1]);

	if (stdin_fd)
		*stdin_fd = stdin_p[1];
	if (stdout_fd)
		*stdout_fd = stdout_p[0];
	if (stderr_fd)
		*stderr_fd = stderr_p[0];

	closed = false;
	data.id = pid;
	return pid;
}

Process::id_type Process::open(const std::vector<string_type> &arguments, const string_type &path, const environment_type *environment) noexcept {
	return open([&arguments, &path, &environment] {
		if (arguments.empty())
			exit(127);

		std::vector<const char *> argv_ptrs;
		argv_ptrs.reserve(arguments.size() + 1);
		for (auto &argument : arguments)
			argv_ptrs.emplace_back(argument.c_str());
		argv_ptrs.emplace_back(nullptr);

		if (!path.empty()) {
			if (chdir(path.c_str()) != 0)
				exit(1);
		}

		if (!environment)
			execv(arguments[0].c_str(), const_cast<char *const *>(argv_ptrs.data()));
		else {
			std::vector<std::string> env_strs;
			std::vector<const char *> env_ptrs;
			env_strs.reserve(environment->size());
			env_ptrs.reserve(environment->size() + 1);
			for (const auto &e : *environment) {
				env_strs.emplace_back(e.first + '=' + e.second);
				env_ptrs.emplace_back(env_strs.back().c_str());
			}
			env_ptrs.emplace_back(nullptr);

			execve(arguments[0].c_str(), const_cast<char *const *>(argv_ptrs.data()), const_cast<char *const *>(env_ptrs.data()));
		}
	});
}

Process::id_type Process::open(const std::string &command, const std::string &path, const environment_type *environment) noexcept {
	return open([&command, &path, &environment] {
		auto command_c_str = command.c_str();
		std::string cd_path_and_command;
		if (!path.empty()) {
			auto path_escaped = path;
			size_t pos = 0;
			// Based on https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
			while ((pos = path_escaped.find('\'', pos)) != std::string::npos) {
				path_escaped.replace(pos, 1, "'\\''");
				pos += 4;
			}
			cd_path_and_command = "cd '" + path_escaped + "' && " + command; // To avoid resolving symbolic links
			command_c_str = cd_path_and_command.c_str();
		}

		if (!environment)
			execl("/bin/sh", "/bin/sh", "-c", command_c_str, nullptr);
		else {
			std::vector<std::string> env_strs;
			std::vector<const char *> env_ptrs;
			env_strs.reserve(environment->size());
			env_ptrs.reserve(environment->size() + 1);
			for (const auto &e : *environment) {
				env_strs.emplace_back(e.first + '=' + e.second);
				env_ptrs.emplace_back(env_strs.back().c_str());
			}
			env_ptrs.emplace_back(nullptr);
			execle("/bin/sh", "/bin/sh", "-c", command_c_str, nullptr, env_ptrs.data());
		}
	});
}

void Process::async_read() noexcept {
	if (data.id <= 0 || (!stdout_fd && !stderr_fd))
		return;

	stdout_stderr_thread = std::thread([this] {
		std::vector<pollfd> pollfds;
		std::bitset<2> fd_is_stdout;
		if (stdout_fd) {
			fd_is_stdout.set(pollfds.size());
			pollfds.emplace_back();
			pollfds.back().fd = fcntl(*stdout_fd, F_SETFL, fcntl(*stdout_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stdout_fd : -1;
			pollfds.back().events = POLLIN;
		}
		if (stderr_fd) {
			pollfds.emplace_back();
			pollfds.back().fd = fcntl(*stderr_fd, F_SETFL, fcntl(*stderr_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stderr_fd : -1;
			pollfds.back().events = POLLIN;
		}
		auto buffer = std::unique_ptr<char[]>(new char[config.buffer_size]);
		bool any_open = !pollfds.empty();
		while (any_open && (poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), -1) > 0 || errno == EINTR)) {
			any_open = false;
			for (size_t i = 0; i < pollfds.size(); ++i) {
				if (pollfds[i].fd >= 0) {
					if (pollfds[i].revents & POLLIN) {
						const ssize_t n = read(pollfds[i].fd, buffer.get(), config.buffer_size);
						if (n > 0) {
							if (fd_is_stdout[i])
								read_stdout(buffer.get(), static_cast<size_t>(n));
							else
								read_stderr(buffer.get(), static_cast<size_t>(n));
						} else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
							pollfds[i].fd = -1;
							continue;
						}
					}
					if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
						pollfds[i].fd = -1;
						continue;
					}
					any_open = true;
				}
			}
		}
	});
}

int Process::get_exit_status() noexcept {
	if (data.id <= 0)
		return -1;

	int exit_status;
	id_type pid;
	do {
		pid = waitpid(data.id, &exit_status, 0);
	} while (pid < 0 && errno == EINTR);

	if (pid < 0 && errno == ECHILD) {
		// PID doesn't exist anymore, return previously sampled exit status (or -1)
		return data.exit_status;
	} else {
		// Store exit status for future calls
		if (exit_status >= 256)
			exit_status = exit_status >> 8;
		data.exit_status = exit_status;
	}

	{
		std::lock_guard<std::mutex> lock(close_mutex);
		closed = true;
	}
	close_fds();

	return exit_status;
}

bool Process::try_get_exit_status(int &exit_status) noexcept {
	if (data.id <= 0)
		return false;

	const id_type pid = waitpid(data.id, &exit_status, WNOHANG);
	if (pid < 0 && errno == ECHILD) {
		// PID doesn't exist anymore, set previously sampled exit status (or -1)
		exit_status = data.exit_status;
		return true;
	} else if (pid <= 0) {
		// Process still running (p==0) or error
		return false;
	} else {
		// store exit status for future calls
		if (exit_status >= 256)
			exit_status = exit_status >> 8;
		data.exit_status = exit_status;
	}

	{
		std::lock_guard<std::mutex> lock(close_mutex);
		closed = true;
	}
	close_fds();

	return true;
}

void Process::close_fds() noexcept {
	if (stdout_stderr_thread.joinable())
		stdout_stderr_thread.join();

	if (stdin_fd)
		close_stdin();
	if (stdout_fd) {
		if (data.id > 0)
			close(*stdout_fd);
		stdout_fd.reset();
	}
	if (stderr_fd) {
		if (data.id > 0)
			close(*stderr_fd);
		stderr_fd.reset();
	}
}

bool Process::write(const char *bytes, size_t n) {
	if (!open_stdin)
		throw std::invalid_argument("Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process.");

	std::lock_guard<std::mutex> lock(stdin_mutex);
	if (stdin_fd) {
		if (::write(*stdin_fd, bytes, n) >= 0) {
			return true;
		} else {
			return false;
		}
	}
	return false;
}

void Process::close_stdin() noexcept {
	std::lock_guard<std::mutex> lock(stdin_mutex);
	if (stdin_fd) {
		if (data.id > 0)
			close(*stdin_fd);
		stdin_fd.reset();
	}
}

void Process::kill(bool force) noexcept {
	std::lock_guard<std::mutex> lock(close_mutex);
	if (data.id > 0 && !closed) {
		if (force)
			::kill(-data.id, SIGTERM);
		else
			::kill(-data.id, SIGINT);
	}
}

void Process::kill(id_type id, bool force) noexcept {
	if (id <= 0)
		return;

	if (force)
		::kill(-id, SIGTERM);
	else
		::kill(-id, SIGINT);
}

void Process::signal(int signum) noexcept {
	std::lock_guard<std::mutex> lock(close_mutex);
	if (data.id > 0 && !closed) {
		::kill(-data.id, signum);
	}
}

} // namespace TinyProcessLib

#else

// clang-format off
#include <windows.h>
// clang-format on
#include <tlhelp32.h>
#include <cstring>
#include <stdexcept>

namespace TinyProcessLib {

Process::Data::Data() noexcept : id(0) {}

// Simple HANDLE wrapper to close it automatically from the destructor.
class Handle {
public:
	Handle() noexcept : handle(INVALID_HANDLE_VALUE) {}
	~Handle() noexcept {
		close();
	}
	void close() noexcept {
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
	}
	HANDLE detach() noexcept {
		HANDLE old_handle = handle;
		handle = INVALID_HANDLE_VALUE;
		return old_handle;
	}
	operator HANDLE() const noexcept { return handle; }
	HANDLE *operator&() noexcept { return &handle; }

private:
	HANDLE handle;
};

//Based on the discussion thread: https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxq1wsj
std::mutex create_process_mutex;

Process::id_type Process::open(const std::vector<string_type> &arguments, const string_type &path, const environment_type *environment) noexcept {
	string_type command;
	for (auto &argument : arguments)
#ifdef UNICODE
		command += (command.empty() ? L"" : L" ") + argument;
#else
		command += (command.empty() ? "" : " ") + argument;
#endif
	return open(command, path, environment);
}

//Based on the example at https://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx.
Process::id_type Process::open(const string_type &command, const string_type &path, const environment_type *environment) noexcept {
	if (open_stdin)
		stdin_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
	if (read_stdout)
		stdout_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
	if (read_stderr)
		stderr_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));

	Handle stdin_rd_p;
	Handle stdin_wr_p;
	Handle stdout_rd_p;
	Handle stdout_wr_p;
	Handle stderr_rd_p;
	Handle stderr_wr_p;

	SECURITY_ATTRIBUTES security_attributes;

	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = nullptr;

	std::lock_guard<std::mutex> lock(create_process_mutex);
	if (stdin_fd) {
		if (!CreatePipe(&stdin_rd_p, &stdin_wr_p, &security_attributes, 0) ||
				!SetHandleInformation(stdin_wr_p, HANDLE_FLAG_INHERIT, 0))
			return 0;
	}
	if (stdout_fd) {
		if (!CreatePipe(&stdout_rd_p, &stdout_wr_p, &security_attributes, 0) ||
				!SetHandleInformation(stdout_rd_p, HANDLE_FLAG_INHERIT, 0)) {
			return 0;
		}
	}
	if (stderr_fd) {
		if (!CreatePipe(&stderr_rd_p, &stderr_wr_p, &security_attributes, 0) ||
				!SetHandleInformation(stderr_rd_p, HANDLE_FLAG_INHERIT, 0)) {
			return 0;
		}
	}

	PROCESS_INFORMATION process_info;
	STARTUPINFO startup_info;

	ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));

	ZeroMemory(&startup_info, sizeof(STARTUPINFO));
	startup_info.cb = sizeof(STARTUPINFO);
	startup_info.hStdInput = stdin_rd_p;
	startup_info.hStdOutput = stdout_wr_p;
	startup_info.hStdError = stderr_wr_p;
	if (stdin_fd || stdout_fd || stderr_fd)
		startup_info.dwFlags |= STARTF_USESTDHANDLES;

	if (config.show_window != Config::ShowWindow::show_default) {
		startup_info.dwFlags |= STARTF_USESHOWWINDOW;
		startup_info.wShowWindow = static_cast<WORD>(config.show_window);
	}

	auto process_command = command;
#ifdef MSYS_PROCESS_USE_SH
	size_t pos = 0;
	while ((pos = process_command.find('\\', pos)) != string_type::npos) {
		process_command.replace(pos, 1, "\\\\\\\\");
		pos += 4;
	}
	pos = 0;
	while ((pos = process_command.find('\"', pos)) != string_type::npos) {
		process_command.replace(pos, 1, "\\\"");
		pos += 2;
	}
	process_command.insert(0, "sh -c \"");
	process_command += "\"";
#endif

	string_type environment_str;
	if (environment) {
#ifdef UNICODE
		for (const auto &e : *environment)
			environment_str += e.first + L'=' + e.second + L'\0';
		environment_str += L'\0';
#else
		for (const auto &e : *environment)
			environment_str += e.first + '=' + e.second + '\0';
		environment_str += '\0';
#endif
	}
	BOOL bSuccess = CreateProcess(nullptr, process_command.empty() ? nullptr : &process_command[0], nullptr, nullptr,
			stdin_fd || stdout_fd || stderr_fd || config.inherit_file_descriptors, // Cannot be false when stdout, stderr or stdin is used
			stdin_fd || stdout_fd || stderr_fd ? CREATE_NO_WINDOW : 0, // CREATE_NO_WINDOW cannot be used when stdout or stderr is redirected to parent process
			environment_str.empty() ? nullptr : &environment_str[0],
			path.empty() ? nullptr : path.c_str(),
			&startup_info, &process_info);

	if (!bSuccess)
		return 0;
	else
		CloseHandle(process_info.hThread);

	if (stdin_fd)
		*stdin_fd = stdin_wr_p.detach();
	if (stdout_fd)
		*stdout_fd = stdout_rd_p.detach();
	if (stderr_fd)
		*stderr_fd = stderr_rd_p.detach();

	closed = false;
	data.id = process_info.dwProcessId;
	data.handle = process_info.hProcess;
	return process_info.dwProcessId;
}

void Process::async_read() noexcept {
	if (data.id == 0)
		return;

	if (stdout_fd) {
		stdout_thread = std::thread([this]() {
			DWORD n;
			std::unique_ptr<char[]> buffer(new char[config.buffer_size]);
			for (;;) {
				BOOL bSuccess = ReadFile(*stdout_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);
				if (!bSuccess || n == 0)
					break;
				read_stdout(buffer.get(), static_cast<size_t>(n));
			}
		});
	}
	if (stderr_fd) {
		stderr_thread = std::thread([this]() {
			DWORD n;
			std::unique_ptr<char[]> buffer(new char[config.buffer_size]);
			for (;;) {
				BOOL bSuccess = ReadFile(*stderr_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);
				if (!bSuccess || n == 0)
					break;
				read_stderr(buffer.get(), static_cast<size_t>(n));
			}
		});
	}
}

int Process::get_exit_status() noexcept {
	if (data.id == 0)
		return -1;

	if (!data.handle)
		return data.exit_status;

	WaitForSingleObject(data.handle, INFINITE);

	DWORD exit_status;
	if (!GetExitCodeProcess(data.handle, &exit_status))
		data.exit_status = -1; // Store exit status for future calls
	else
		data.exit_status = static_cast<int>(exit_status); // Store exit status for future calls

	{
		std::lock_guard<std::mutex> lock(close_mutex);
		CloseHandle(data.handle);
		data.handle = nullptr;
		closed = true;
	}
	close_fds();

	return data.exit_status;
}

bool Process::try_get_exit_status(int &exit_status) noexcept {
	if (data.id == 0)
		return false;

	if (!data.handle) {
		exit_status = data.exit_status;
		return true;
	}

	DWORD wait_status = WaitForSingleObject(data.handle, 0);
	if (wait_status == WAIT_TIMEOUT)
		return false;

	DWORD exit_status_tmp;
	if (!GetExitCodeProcess(data.handle, &exit_status_tmp))
		exit_status = -1;
	else
		exit_status = static_cast<int>(exit_status_tmp);
	data.exit_status = exit_status; // Store exit status for future calls

	{
		std::lock_guard<std::mutex> lock(close_mutex);
		CloseHandle(data.handle);
		data.handle = nullptr;
		closed = true;
	}
	close_fds();

	return true;
}

void Process::close_fds() noexcept {
	if (stdout_thread.joinable())
		stdout_thread.join();
	if (stderr_thread.joinable())
		stderr_thread.join();

	if (stdin_fd)
		close_stdin();
	if (stdout_fd) {
		if (*stdout_fd != nullptr)
			CloseHandle(*stdout_fd);
		stdout_fd.reset();
	}
	if (stderr_fd) {
		if (*stderr_fd != nullptr)
			CloseHandle(*stderr_fd);
		stderr_fd.reset();
	}
}

bool Process::write(const char *bytes, size_t n) {
	if (!open_stdin)
		throw std::invalid_argument("Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process.");

	std::lock_guard<std::mutex> lock(stdin_mutex);
	if (stdin_fd) {
		DWORD written;
		BOOL bSuccess = WriteFile(*stdin_fd, bytes, static_cast<DWORD>(n), &written, nullptr);
		if (!bSuccess || written == 0) {
			return false;
		} else {
			return true;
		}
	}
	return false;
}

void Process::close_stdin() noexcept {
	std::lock_guard<std::mutex> lock(stdin_mutex);
	if (stdin_fd) {
		if (*stdin_fd != nullptr)
			CloseHandle(*stdin_fd);
		stdin_fd.reset();
	}
}

//Based on http://stackoverflow.com/a/1173396
void Process::kill(bool /*force*/) noexcept {
	std::lock_guard<std::mutex> lock(close_mutex);
	if (data.id > 0 && !closed) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot) {
			PROCESSENTRY32 process;
			ZeroMemory(&process, sizeof(process));
			process.dwSize = sizeof(process);
			if (Process32First(snapshot, &process)) {
				do {
					if (process.th32ParentProcessID == data.id) {
						HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process.th32ProcessID);
						if (process_handle) {
							TerminateProcess(process_handle, 2);
							CloseHandle(process_handle);
						}
					}
				} while (Process32Next(snapshot, &process));
			}
			CloseHandle(snapshot);
		}
		TerminateProcess(data.handle, 2);
	}
}

//Based on http://stackoverflow.com/a/1173396
void Process::kill(id_type id, bool /*force*/) noexcept {
	if (id == 0)
		return;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot) {
		PROCESSENTRY32 process;
		ZeroMemory(&process, sizeof(process));
		process.dwSize = sizeof(process);
		if (Process32First(snapshot, &process)) {
			do {
				if (process.th32ParentProcessID == id) {
					HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process.th32ProcessID);
					if (process_handle) {
						TerminateProcess(process_handle, 2);
						CloseHandle(process_handle);
					}
				}
			} while (Process32Next(snapshot, &process));
		}
		CloseHandle(snapshot);
	}
	HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, id);
	if (process_handle)
		TerminateProcess(process_handle, 2);
}

} // namespace TinyProcessLib

#endif