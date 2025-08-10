/*
 * Entry point for the whole application
 * Depends on fuzzylite and pagmo libraries!
 */


#include "pm_solver.h"

#include <fl/Headers.h>

#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/problems/schwefel.hpp>

using Stats = std::tuple<
	double, // strength
	double, // constitution
	double, // intelligence
	double  // refinement
	>;

using Inclinations = std::tuple<
	double, // PhysicalInclination
	double // MentalInclination
	>;

const std::unordered_map<
	std::string, // job name
	Stats // stat changes after taking this action
> actions{
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


template <typename... T>
std::tuple<T...> tuple_sum(const std::tuple<T...>& a, const std::tuple<T...>& b) {
	return std::apply([&b](const T&... av) {
		return std::apply([&](const T&... bv) {
			return std::make_tuple((av + bv)...);
			}, b);
		}, a);
}

Stats sum_stats(const Stats& a, const Stats& b)
{
	return tuple_sum(a, b);
}

std::string choose_action(fl::Engine* engine, Inclinations inclinations, Stats stats)
{
	// Load the specimen into the engine
	engine->getInputVariable("PhysicalInclination")->setValue(std::get<0>(inclinations));
	engine->getInputVariable("MentalInclination")->setValue(std::get<1>(inclinations));

	engine->getInputVariable("strength")->setValue(std::get<0>(stats));
	engine->getInputVariable("constitution")->setValue(std::get<1>(stats));
	engine->getInputVariable("intelligence")->setValue(std::get<2>(stats));
	engine->getInputVariable("refinement")->setValue(std::get<3>(stats));

	// Get action priorities
	engine->process();

	fl::scalar max_priority{ -1'000'000 };
	std::string chosen_action_name;

	for (const auto action_priority_var : engine->outputVariables())
	{
		std::string action_name{ action_priority_var->getName() };
		fl::scalar action_priority{ action_priority_var->getValue() };

		std::cout << "Action " << action_name << " priority: " << action_priority << "\n";

		if (action_priority > max_priority)
		{
			max_priority = action_priority;
			chosen_action_name = action_name;
		}
	}

	return chosen_action_name;
}

std::unique_ptr<fl::Engine> init()
{
	// Initialize the engine
	std::string path{ "C:\\projects\\pm_solver\\Trivial.fll" };
	std::unique_ptr<fl::Engine> engine{ fl::FllImporter().fromFile(path) };
	// Checking for errors in the engine loading.
	std::string status;
	if (not engine->isReady(&status))
	{
		throw fl::Exception("[engine error] engine is not ready: \n" + status);
	}

	// We want to see the details of the engine processing.
	fuzzylite::fuzzylite::setDebugging(true);

	return engine;
}

int main()
{
	// Trivial case draft

	auto engine = init();

	// Initialize a specimen
	Stats stats{ 0.0, 0.0, 0.0, 0.0 };
	// Specifically not 1 because it will not fall into the "high" term.
	// TODO: setup the defaults for output values for the cases when no rules fire and set their strict values.
	Inclinations inclinations{ 0.67, 0.33 };
	
	std::string chosen_action_name = choose_action(engine.get(), inclinations, stats);

	std::cout << "Chosen action: " << chosen_action_name << "\n";

	if (chosen_action_name.empty())
	{
		std::cout << "No action chosen, exiting.\n";
		return 0;
	}

	Stats stats_diff = actions.at(chosen_action_name);

	stats = sum_stats(stats, stats_diff);


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
