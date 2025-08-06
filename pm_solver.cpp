// pm_solver.cpp: определяет точку входа для приложения.
//

#include "pm_solver.h"

#include <fl/Headers.h>

#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/problems/schwefel.hpp>

// stats: str, con, intelligence, refinement

std::unordered_map<
	std::string, // job name
	std::tuple<double, double, double, double> // (str, con, intelligence, refinement)
> jobs{
	/*
	* https://princessmaker.fandom.com/wiki/Hunter_(PM2)
	* Stats/Skills Affected
		Constitution: +1/day
		Combat Skill: Random raise, + 0 to 1/day
		Refinement: -1/day
		Sin: Random raise, +0 to 1/day
		Stress: +3/day
		Hidden Stat:
		Maternal Instinct: -1/day
		Stats Required
		Constitution, Intelligence.
	*/
	{ "Hunting",    { 0.00, 0.01, 0.00, -0.01 } },
	/*
	* https://princessmaker.fandom.com/wiki/Lumberjack_(PM2)
	* Statistics Affected
Strength: +2/day

Refinement: -2/day

Stress: +4/day

Statistics Required
Constitution, strength.
	*/
	{ "Lumberjack", { 0.02, 0.00, 0.00, -0.02 } },
	/*
	* https://princessmaker.fandom.com/wiki/Science_Class_(PM2)
	* Charts
Science	Novice	Adept	Expert	Master
Tuition/Day	30 G	40 G	50 G	60 G
Intelligence	+1 to 4	+2 to 6	+3 to 8	+4 to 12
Faith	-0	-0 to 1	-0 to 2	-0 to 3
Magical Defense	-0	-0 to 1	-0 to 1	-0 to 1

	*/
	{ "ScienceClass", { 0.0, 0.0, 0.20, 0.0 } },
	/*
	https://princessmaker.fandom.com/wiki/Protocol_Class_(PM2)
	Charts
Protocol	Novice	Adept	Expert	Master
Tuition/Day	40 G	50 G	60 G	70 G
Decorum	+1	+1 to 2	+1 to 3	+1 to 4
Refinement	+1	+1 to 2	+1 to 3	+1 to 4

	*/
	{ "MannersClass", { 0.0, 0.0, 0.0, 2.0 } },
};

int main()
{

	// Trivial case draft

	std::string path{ "C:\\projects\\pm_solver\\Trivial.fll"};
	auto engine{ std::make_unique<fl::Engine>(fl::FllImporter().fromFile(path)) };

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
