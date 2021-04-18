/* GRObjectPool.h */
#pragma once

#include <functional>
#include <mutex>
#include <queue>

template <class TObject>
class GRObjectPool {
	std::function<std::shared_ptr<TObject>(void)> _constructor = nullptr;
	std::function<void(std::shared_ptr<TObject>)> _on_return = nullptr;
	std::queue<std::shared_ptr<TObject> > val_queue;
	mutable std::recursive_mutex ts_lock;
	bool _is_dynamic = true;
	int _static_size = 0;

public:
	class PoolObject {
		friend GRObjectPool<TObject>;
		std::shared_ptr<TObject> obj;
		GRObjectPool<TObject> *owner;

		PoolObject(GRObjectPool<TObject> *_owner, std::shared_ptr<TObject> object) {
			owner = _owner;
			obj = object;
		}

	public:
		~PoolObject() {
			reset();
		}

		void reset() {
			if (owner) {
				owner->put(obj);
				obj = nullptr;
				owner = nullptr;
			}
		}

		inline TObject *operator->() {
			return obj.get();
		}

		inline TObject *operator*() {
			return obj.get();
		}

		inline const TObject *operator->() const {
			return obj.get();
		}

		inline const TObject *operator*() const {
			return obj.get();
		}

		inline const TObject *ptr() const {
			return obj.get();
		}

		inline TObject *ptr() {
			return obj.get();
		}
	};

	void clear() {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		while (val_queue.size()) {
			val_queue.pop();
		}
	}

	std::shared_ptr<TObject> get_unscoped() {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		if (val_queue.size()) {
			std::shared_ptr<TObject> obj = val_queue.front();
			val_queue.pop();
			return obj;
		} else {
			if (_is_dynamic) {
				return _constructor();
			} else {
				return std::shared_ptr<TObject>();
			}
		}
	}

	PoolObject get() {
		return PoolObject(this, get_unscoped());
	}

	void put(std::shared_ptr<TObject> obj) {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		if (!_is_dynamic && val_queue.size() >= _static_size)
			return;
		if (_on_return) {
			_on_return(obj);
		}
		val_queue.push(obj);
	}

	int size() {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		return (int)val_queue.size();
	}

	GRObjectPool(std::function<std::shared_ptr<TObject>(void)> constructor, std::function<void(std::shared_ptr<TObject>)> on_return = nullptr, bool is_dynamic = true, int static_size = 0) {
		_constructor = constructor;
		_on_return = on_return;
		_is_dynamic = is_dynamic;
		_static_size = static_size;

		if (!is_dynamic) {
			for (int i = 0; i < static_size; i++) {
				val_queue.push(_constructor());
			}
		}
	}
};