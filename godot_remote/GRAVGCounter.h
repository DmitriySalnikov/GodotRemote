/* GRAVGCounter.h */
#pragma once

#include "iterable_queue.h"
#include <functional>
#include <mutex>

template <typename TQueue, typename TValue>
class GRAVGCounter {
	TValue avg_val = 0, min_val = 0, max_val = 0;
	uint32_t queue_size;
	std::function<TValue(TValue)> modifier = nullptr;
	iterable_queue<TQueue> val_queue;
	mutable std::recursive_mutex ts_lock;

public:
	inline TValue get_avg() const {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		return avg_val;
	}

	inline TValue get_min() const {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		return min_val;
	}

	inline TValue get_max() const {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		return max_val;
	}

	void reset() {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		avg_val = min_val = max_val = 0;
		val_queue = iterable_queue<TQueue>();
	}

	void update(TQueue val, uint32_t _max_queue_size = 0) {
		std::lock_guard<std::recursive_mutex> _scoped_lock_guard(ts_lock);
		_max_queue_size = _max_queue_size > 0 ? _max_queue_size : queue_size;

		val_queue.push(val);
		while (val_queue.size() > _max_queue_size) {
			val_queue.pop();
		}

		if (val_queue.size() > 0) {
			double sum = 0;
			min_val = (TValue)val_queue.back();
			max_val = (TValue)val_queue.back();

			for (TQueue i : val_queue) {
				sum += i;
				if (i < min_val)
					min_val = (TValue)i;
				else if (i > max_val)
					max_val = (TValue)i;
			}
			avg_val = modifier(TValue(sum / val_queue.size()));
			min_val = modifier(TValue(min_val));
			max_val = modifier(TValue(max_val));
		} else {
			avg_val = 0;
			min_val = 0;
			max_val = 0;
		}
	}

	GRAVGCounter(const GRAVGCounter<TQueue, TValue> &other) {
		avg_val = other.avg_val;
		min_val = other.min_val;
		max_val = other.max_val;
		queue_size = other.queue_size;
		modifier = other.modifier;
	}

	GRAVGCounter(std::function<TValue(TValue)> _modifier) {
		modifier = _modifier;
		queue_size = 1;
	}

	GRAVGCounter(std::function<TValue(TValue)> _modifier, uint32_t queue_max_size) {
		modifier = _modifier;
		queue_size = queue_max_size;
	}
};