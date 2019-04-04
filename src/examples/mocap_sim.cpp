#include "salsa/salsa.h"
#include "salsa/misc.h"

#include "multirotor_sim/simulator.h"

using namespace salsa;
using namespace std;


int main()
{
  Simulator sim(true);
  sim.load(imu_mocap());
#ifndef NDEBUG
  sim.tmax_ = 10;
#endif

  Salsa salsa;
  salsa.init(default_params("/tmp/Salsa/MocapSimulation/"));

  sim.register_estimator(&salsa);

  Logger true_state_log(salsa.log_prefix_ + "Truth.log");

  while (sim.run())
  {
    true_state_log.log(sim.t_);
    true_state_log.logVectors(sim.state().X.arr(), sim.state().v, sim.accel_bias_,
                              sim.gyro_bias_, Vector2d{sim.clock_bias_, sim.clock_bias_rate_});
  }
}
