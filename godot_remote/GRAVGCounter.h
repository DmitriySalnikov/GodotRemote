/* GRAVGCounter.h */
#pragma once

#include "iterable_queue.h"
#include <functional>

template <typename TQueue, typename TValue>
class GRAVGCounter {
	TValue avg_val = 0, min_val = 0, max_val = 0;
	uint32_t queue_size;
	std::function<TValue(TValue)> modifier = nullptr;
	iterable_queue<TQueue> val_queue;

public:
	inline TValue get_avg() const {
		return avg_val;
	}

	inline TValue get_min() const {
		return min_val;
	}

	inline TValue get_max() const {
		return max_val;
	}

	void reset() {
		avg_val = min_val = max_val = 0;
		val_queue = iterable_queue<TQueue>();
	}

	void update(TQueue val, uint32_t _max_queue_size = 0) {
		_max_queue_size = _max_queue_size > 0 ? _max_queue_size : queue_size;

		val_queue.push(val);
		while (val_queue.size() > _max_queue_size) {
			val_queue.pop();
		}

		if (val_queue.size() > 0) {
			double sum = 0;
			min_val = (float)val_queue.back();
			max_val = (float)val_queue.back();

			for (TQueue i : val_queue) {
				sum += i;
				if (i < min_val)
					min_val = (float)i;
				else if (i > max_val)
					max_val = (float)i;
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

	GRAVGCounter(std::function<TValue(TValue)> _modifier) {
		modifier = _modifier;
		queue_size = 1;
	}

	GRAVGCounter(std::function<TValue(TValue)> _modifier, uint32_t queue_max_size) {
		modifier = _modifier;
		queue_size = queue_max_size;
	}
};