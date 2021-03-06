#ifndef _VILLE_CONNECTEE_MOVABLE_BRIDGE_H
#define _VILLE_CONNECTEE_MOVABLE_BRIDGE_H

#include <Arduino.h>
#include "stepper_driver/interface.hpp"

#include <type_traits>
#include <thread>
#include <atomic>
//#include <pthread.h>

#include "debugging.hpp"

template<typename StepperDriverT1, typename StepperDriverT2>
class MovableBridge {
	static_assert(std::is_base_of<StepperDriver::IStepperDriver, StepperDriverT1>::value, "StepperDriverT1 must inherit from StepperDriver::IStepperDriver");
	static_assert(std::is_base_of<StepperDriver::IStepperDriver, StepperDriverT2>::value, "StepperDriverT2 must inherit from StepperDriver::IStepperDriver"); 
private:
#	define callSteppersMethod(method, ...) stepperDriver1.method(__VA_ARGS__); stepperDriver2.method(__VA_ARGS__);	// Warning! Unsafe macro! Doesn't prevent expansion!

	StepperDriverT1 stepperDriver1;
	StepperDriverT2 stepperDriver2;

	float openAngle;

	template<typename StepperDriverT>
	static void runCallback(StepperDriverT& stepperDriver, std::atomic<bool>& hasThreadFinished) {
		hasThreadFinished = false;
		stepperDriver.run();
		delay(40000);
		hasThreadFinished = true;
	}

	void run2() {
		while (!stepperDriver1.isOpen() || !stepperDriver2.isOpen()) {
			if (!stepperDriver1.isOpen())
				stepperDriver1.nextStep();
			if (!stepperDriver2.isOpen())
				stepperDriver2.nextStep();
		}
	}

	void run() {
		// std::atomic<bool> hasThreadFinished;
		//std::thread t1(&runCallback<StepperDriverT1>, std::ref(stepperDriver1), std::ref(hasThreadFinished));
		//while (!hasThreadFinished);
		std::thread t1(&StepperDriverT1::run, &stepperDriver1);
		std::thread t2(&StepperDriverT2::run, &stepperDriver2);
		//delay(60000);
		t1.join();
		t2.join();
		//delay(10000);
		//t2.join();
	}
public:
	using PinoutDescriptor1 = typename StepperDriverT1::PinoutDescriptor;
	using PinoutDescriptor2 = typename StepperDriverT2::PinoutDescriptor;

	float getOpenAngle() const { return openAngle; }
	void setOpenAngle(float angle) { openAngle = angle; }
	uint_fast32_t getSpeedAngle() const { return stepperDriver1.getSpeedAngle(); } // Remember, stepperDriver1.getSpeed() == stepperDriver2.getSpeed().
	void setSpeedAngle(uint_fast32_t speed) { callSteppersMethod(setSpeedAngle, speed); }

	MovableBridge(const typename StepperDriverT1::PinoutDescriptor& pd1, uint_fast32_t stepper1RevolutionSteps, const typename StepperDriverT2::PinoutDescriptor& pd2, uint_fast32_t stepper2RevolutionSteps, float speed = 4*PI)
		: stepperDriver1(pd1, stepper1RevolutionSteps, speed), stepperDriver2(pd2, stepper2RevolutionSteps, speed) {}

	void open() { callSteppersMethod(setTargetAngle, openAngle); run(); }

	void close() { callSteppersMethod(setTargetAngle, 0); run(); }

private:
#	undef callSteppersMethod
};

#endif
