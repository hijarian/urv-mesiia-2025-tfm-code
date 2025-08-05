// pm_solver.cpp: определяет точку входа для приложения.
//

#include "pm_solver.h"

#include <fl/Headers.h>

#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/problems/schwefel.hpp>

int main()
{

	/// Fuzzylite smoke test BEGIN

	std::string path{ "C:\\projects\\pm_solver\\ObstacleAvoidance.fll"};
	fl::Engine* engine = fl::FllImporter().fromFile(path);
	std::string status;

	if (not engine->isReady(&status))
	{
		throw fl::Exception("[engine error] engine is not ready: \n" + status);
	}

	fl::InputVariable* obstacle = engine->getInputVariable("obstacle");
	fl::OutputVariable* steer = engine->getOutputVariable("mSteer");

	for (int i = 0; i <= 50; ++i)
	{
		fl::scalar location = obstacle->getMinimum() + i * (obstacle->range() / 50);
		obstacle->setValue(location);
		engine->process();

		std::cout <<
			"obstacle.input = "
			<< fl::Op::str(location)
			<< " => "
			<< "steer.output = "
			<< fl::Op::str(steer->getValue())
			<< "\n";
	}
	/// Fuzzylite smoke test END

	/// Pagmo smoke test BEGIN

	pagmo::problem prob(pagmo::schwefel(30));

	pagmo::algorithm algo(pagmo::sade(100));

	pagmo::archipelago archi(16u, algo, prob, 20u);

	archi.evolve(10);

	archi.wait_check();

	for (const auto& isl : archi)
	{
		std::cout << isl.get_population().champion_f()[0] << "\n";
	}

	/// Pagmo smoke test END

	return 0;
}
