#pragma once

namespace bh {

// Fixed-timestep accumulator for the 20 Hz sim (client-side driver).
// Render interpolates with alpha(); sim work happens in whole ticks only.
class TickStepper {
 public:
  explicit TickStepper(double tickSeconds) : tick_(tickSeconds) {}

  // Advance wall-clock; returns how many sim ticks to execute this frame.
  int advance(double frameDt) {
    acc_ += frameDt;
    if (acc_ > 0.25) acc_ = 0.25;  // spiral-of-death guard
    int steps = 0;
    while (acc_ >= tick_ && steps < 8) {
      acc_ -= tick_;
      ++steps;
    }
    return steps;
  }

  double alpha() const { return acc_ / tick_; }

 private:
  double tick_;
  double acc_ = 0.0;
};

}  // namespace bh
