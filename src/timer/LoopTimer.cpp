#include "LoopTimer.h"

#include <sys/resource.h>
#include <unistd.h>

namespace Sai2Common {

LoopTimer::LoopTimer(double frequency, unsigned int initial_wait_nanoseconds) {
	resetLoopFrequency(frequency);
	reinitializeTimer(initial_wait_nanoseconds);
}

void LoopTimer::resetLoopFrequency(double frequency) {
	ns_update_interval_ =
		std::chrono::nanoseconds(static_cast<unsigned int>(1e9 / frequency));
}

void LoopTimer::reinitializeTimer(unsigned int initial_wait_nanoseconds) {
	update_counter_ = 0;
	auto ns_initial_wait = std::chrono::nanoseconds(initial_wait_nanoseconds);
	t_curr_ = std::chrono::high_resolution_clock::now();
	t_start_ = t_curr_ + ns_initial_wait;
	t_next_ = t_start_;
	overtime_loops_counter_ = 0;
	average_overtime_ms_ = 0.0;
	running_ = true;
}

bool LoopTimer::waitForNextLoop() {
	update_counter_++;
	bool return_val = true;
	t_curr_ = std::chrono::high_resolution_clock::now();

	if (t_curr_ < t_next_) {
		t_curr_ = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(t_next_ - t_curr_);
		t_next_ += ns_update_interval_;
	} else {
		// calculate overtime
		auto t_overtime_ms =
			std::chrono::duration<double>(t_curr_ - t_next_).count() * 1e3;
		++overtime_loops_counter_;
		average_overtime_ms_ +=
			(t_overtime_ms - average_overtime_ms_) / overtime_loops_counter_;

		// if monitor activated and conditions satisfied, throw an error
		if (overtime_monitor_enabled_) {
			if (t_overtime_ms > overtime_monitor_threshold_ms_) {
				return_val = false;
				if (overtime_monitor_print_warning_) {
					std::cout
						<< "LoopTimer. Overtime over the allowed threshold "
						   "detected. Current overtime: "
						<< t_overtime_ms
						<< " ms, threshold: " << overtime_monitor_threshold_ms_
						<< " ms" << std::endl;
				}
			}
			if (average_overtime_ms_ > overtime_monitor_average_threshold_ms_) {
				return_val = false;
				if (overtime_monitor_print_warning_) {
					std::cout << "LoopTimer. Average overtime over the "
								 "allowed threshold "
								 "detected. Current average overtime: "
							  << average_overtime_ms_ << " ms, threshold: "
							  << overtime_monitor_average_threshold_ms_ << " ms"
							  << std::endl;
				}
			}
			if ((double)overtime_loops_counter_ / update_counter_ * 100.0 >
				overtime_monitor_percentage_allowed_) {
				return_val = false;
				if (overtime_monitor_print_warning_) {
					std::cout << "LoopTimer. Percentage of overtime over "
								 "the allowed "
								 "threshold detected. Current percentage: "
							  << (double)overtime_loops_counter_ /
									 update_counter_ * 100.0
							  << " %, threshold: "
							  << overtime_monitor_percentage_allowed_ << " %"
							  << std::endl;
				}
			}
		} else {
			return_val = false;
		}
		t_curr_ = std::chrono::high_resolution_clock::now();
		t_next_ = t_curr_ + ns_update_interval_;
	}
	return return_val;
}

unsigned long long LoopTimer::elapsedCycles() { return update_counter_; }

double LoopTimer::elapsedTime() {
	return std::chrono::duration<double>(t_next_ - t_start_).count();
}

double LoopTimer::elapsedSimTime() {
	return elapsedCycles() *
		   std::chrono::duration<double>(ns_update_interval_).count();
}

void LoopTimer::enableOvertimeMonitoring(
	const double max_overtime_ms, const double max_average_overtime_ms,
	const double percentage_overtime_loops_allowed, const bool print_warning) {
	overtime_monitor_enabled_ = true;
	overtime_monitor_threshold_ms_ = max_overtime_ms;
	overtime_monitor_average_threshold_ms_ = max_average_overtime_ms;
	overtime_monitor_percentage_allowed_ = percentage_overtime_loops_allowed;
	overtime_monitor_print_warning_ = print_warning;
}

void LoopTimer::printInfoPostRun() {
	std::cout << "Elapsed time                         : " << elapsedTime()
			  << " s\n";
	std::cout << "Time that should have been elapsed   : " << elapsedSimTime()
			  << " s\n";
	std::cout << "Elapsed timer cycles                 : " << elapsedCycles()
			  << "\n";
	std::cout << "Actual running frequency             : "
			  << elapsedCycles() / elapsedTime() << "Hz\n";
	std::cout << "Number of overtime ticks             : "
			  << overtime_loops_counter_ << "\n";
	std::cout << "Percentage of overtime ticks         : "
			  << (double)overtime_loops_counter_ / elapsedCycles() * 100.0
			  << " %\n";
	std::cout << "Average overtime on overtime ticks   : "
			  << average_overtime_ms_ << " ms\n";
}

void LoopTimer::run(void (*userCallback)(void)) {
	reinitializeTimer(ns_update_interval_.count());

	running_ = true;
	while (running_) {
		waitForNextLoop();
		userCallback();
	}
}

void LoopTimer::stop() { running_ = false; }

void LoopTimer::setThreadHighPriority() {
	pid_t pid = getpid();
	int priority_status = setpriority(PRIO_PROCESS, pid, -19);
	if (priority_status) {
		printWarning(
			"setThreadHighPriority. Failed to set priority. You may need to "
			"run as root.");
	}
}

// static void LoopTimer::setThreadRealTime(const int MAX_SAFE_STACK = 8*1024) {
//     // Declare ourself as a real time task, priority 49.
//     // PRREMPT_RT uses priority 50
//     // for kernel tasklets and interrupt handler by default
//     struct sched_param param;
//     param.sched_priority = 49;
//    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
//         perror("sched_setscheduler failed");
//         exit(-1);
//     }

//     // Lock memory
//     if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
//         perror("mlockall failed");
//         exit(-2);
//     }

//     // Pre-fault our stack
//     //int MAX_SAFE_STACK = 8*1024;
//     unsigned char dummy[MAX_SAFE_STACK];
//     memset(dummy, 0, MAX_SAFE_STACK);
// }

}  // namespace Sai2Common