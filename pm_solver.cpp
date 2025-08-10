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

std::string choose_action(fl::Engine* engine, const Inclinations& inclinations, const Stats& stats)
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

	const auto& output_vars = engine->outputVariables();
	auto it = std::max_element(
		output_vars.begin(), output_vars.end(),
		[](const auto* a, const auto* b) {
			// defaulting to 0 if the value is NaN
            const auto left_priority = std::isnan(a->getValue()) ? 0.0 : a->getValue();
            const auto right_priority = std::isnan(b->getValue()) ? 0.0 : b->getValue();

			std::cout << "Comparing " << a->getName() << " with value " << left_priority
				<< " and " << b->getName() << " with value " << right_priority << "\n";
			return left_priority < right_priority;
		}
	);

	// Either return the name of the action with the highest priority,
	// or the first action if no rules fired (i.e., all priorities are 0).
	std::string chosen_action_name = (it != output_vars.end())
		? (*it)->getName()
		: (*output_vars.begin())->getName();

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

std::string single_step(Stats& stats, const Inclinations& inclinations, fl::Engine* engine)
{
	// Choose an action based on the current stats and inclinations
	std::string chosen_action_name = choose_action(engine, inclinations, stats);
	if (chosen_action_name.empty())
	{
		std::cout << "No action chosen, exiting.\n";
		return "";
	}
	std::cout << "Chosen action: " << chosen_action_name << "\n";
	// Apply the effects of the chosen action
	Stats stats_diff = actions.at(chosen_action_name);
	stats = sum_stats(stats, stats_diff);

	return chosen_action_name;
}

/** the lower the better (conforming to pagmo2 conventions) */
double fitness(const Stats& stats)
{
	// demo fitness: desirable strength is 0.05+
	return 0.05 - std::get<0>(stats);
}

constexpr int T = 3; // number of steps to take

std::pair<std::vector<std::string>, double> simulate(const Inclinations& inclinations, fl::Engine* engine)
{
	// Initialize a specimen
	Stats stats{ 0.0, 0.0, 0.0, 0.0 };
	std::vector<std::string> path{};

	engine->restart();

	for (int i = 0; i < T; ++i)
	{
		std::cout << "Step " << i + 1 << ":\n";
		auto step = single_step(stats, inclinations, engine);

		std::cout << "Current stats: "
			<< "Strength: " << std::get<0>(stats) << ", "
			<< "Constitution: " << std::get<1>(stats) << ", "
			<< "Intelligence: " << std::get<2>(stats) << ", "
			<< "Refinement: " << std::get<3>(stats) << "\n";

		path.push_back(step);
	}

	return std::make_pair(path, fitness(stats));
}

using SimulationResult = std::pair<
	/** Sequence of action names */
	std::vector<std::string>,
	/** Fitness value */
	double>;

void print_simulation_result(const SimulationResult& result)
{
	std::cout << "Simulation Result:\n";
	std::cout << "Fitness: " << result.second << "\n";
	std::cout << "Actions taken:\n";
	for (const auto& action : result.first)
	{
		std::cout << "- " << action << "\n";
	}
}

int main()
{
	auto engine = init();

	SimulationResult physical_specimen_result = simulate({ 0.67, 0.33 }, engine.get());

	print_simulation_result(physical_specimen_result);

	SimulationResult mental_specimen_result = simulate({ 0.33, 0.67 }, engine.get());

	print_simulation_result(mental_specimen_result);


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
