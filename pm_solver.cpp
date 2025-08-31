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
    int, //0 strength
    int, // constitution
    int, // intelligence
    int, // refinement
    int, //4 charisma
    int, // morality
    int, // faith
    int, // sin
    int, // sensitivity

    int, //9 combat skill
    int, // combat attack
    int, // combat defense
    int, // magic skill
    int, // magic attack
    int, //14 magic defense
    int, // decorum
	int, // artistry skill
    int, // eloquence
    int, // cooking skill
    int, //19 cleaning skill
    int // temperament
>;

void print_stats(const Stats &s) {
    printf("Stats changes: \n");
    printf("  str: %+d, con: %+d, int: %+d, ref: %+d, cha: %+d, mor: %+d, fai: %+d, sin: %+d, sen: %+d\n",
        std::get<0>(s), std::get<1>(s), std::get<2>(s), std::get<3>(s), std::get<4>(s),
        std::get<5>(s), std::get<6>(s), std::get<7>(s), std::get<8>(s));
    printf("  cs: %+d, ca: %+d, cd: %+d, ms: %+d, ma: %+d, md: %+d, dec: %+d, art: %+d, elo: %+d, coo: %+d, cle: %+d, tem: %+d\n",
        std::get<9>(s), std::get<10>(s), std::get<11>(s), std::get<12>(s), std::get<13>(s),
        std::get<14>(s), std::get<15>(s), std::get<16>(s), std::get<17>(s), std::get<18>(s),
        std::get<19>(s), std::get<20>(s));
}

using Inclinations = std::tuple<
    double, // fighting
    double, // magic
    double, // housekeeping
    double, // artistry
    double // sinfulness
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
    { "Hunting", // +name of the action
        { /*str*/0, /*con*/1, /*int*/0, /*ref*/-1, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/1, /*sen*/0, // 9 stats changes
		   /*cs*/1,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        } 
    },
    /*
    * https://princessmaker.fandom.com/wiki/Lumberjack_(PM2)
    * Statistics Affected
Strength: +2/day
Refinement: -2/day
Stress: +4/day
Statistics Required
Constitution, strength.
    */
    { "Lumberjack", // +name of the action
        { /*str*/2, /*con*/0, /*int*/0, /*ref*/-2, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
           /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "Housework", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/-2, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/+1, /*cle*/+1, /*tem*/+1 // 12 skills changes
        }
    },
    { "Babysitting", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/-1, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/1, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Church", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/1, /*fai*/2, /*sin*/-2, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Farming", // +name of the action
        { /*str*/+1, /*con*/+1, /*int*/0, /*ref*/-1, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Innkeeping", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/-1,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/1, /*tem*/0 // 12 skills changes
    }
},
    { "Restaurant", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/-1,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/+1, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Salon", // +name of the action
        { /*str*/-1, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/+1, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Masonry", // +name of the action
        { /*str*/0, /*con*/2, /*int*/0, /*ref*/0, /*cha*/-1, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Graveyard", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/-1, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/+1, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/1, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Bar", // +name of the action
        { /*str*/0, /*con*/0, /*int*/-2, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/+1, /*coo*/+1, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "Tutoring", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/-1, /*mor*/+1, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
},
    { "SleazyBar", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/2, /*mor*/-3, /*fai*/-3, /*sin*/2, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/-1 // 12 skills changes
    }
},
    { "Cabaret", // +name of the action
        { /*str*/0, /*con*/0, /*int*/-1, /*ref*/-2, /*cha*/3, /*mor*/0, /*fai*/0, /*sin*/1, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/-1 // 12 skills changes
    }
},
    { "DanceClass", // +name of the action
        { /*str*/0, /*con*/1, /*int*/0, /*ref*/0, /*cha*/1, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/1, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "FencingClass", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
        /*cs*/1,  /*ca*/1,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "FightingClass", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
        /*cs*/1,  /*ca*/0,  /*cd*/1,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "MagicClass", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/1, /*ma*/2, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "PaintingClass", // +name of the action
        { /*str*/0, /*con*/0, /*int*/0, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/1, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/2, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "PoetryClass", // +name of the action
    { /*str*/0, /*con*/0, /*int*/1, /*ref*/1, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/1, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/1, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
    { "StrategyClass", // +name of the action
    { /*str*/0, /*con*/0, /*int*/2, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/-1, // 9 stats changes
        /*cs*/1,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        }
    },
        /*
    * https://princessmaker.fandom.com/wiki/Science_Class_(PM2)
    * Charts
Science	Novice	Adept	Expert	Master
Tuition/Day	30 G	40 G	50 G	60 G
Intelligence	+1 to 4	+2 to 6	+3 to 8	+4 to 12
Faith	-0	-0 to 1	-0 to 2	-0 to 3
Magical Defense	-0	-0 to 1	-0 to 1	-0 to 1

    */
    { "ScienceClass",  // +name of the action
{ /*str*/0, /*con*/0, /*int*/2, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/-1, /*sin*/0, /*sen*/0, // 9 stats changes
/*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/-1, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
}
 },
    { "TheologyClass", // +name of the action
    { /*str*/0, /*con*/0, /*int*/1, /*ref*/0, /*cha*/0, /*mor*/0, /*fai*/1, /*sin*/0, /*sen*/0, // 9 stats changes
    /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/1, /*dec*/0, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
    }
    },
    /*
    https://princessmaker.fandom.com/wiki/Protocol_Class_(PM2)
    Charts
Protocol	Novice	Adept	Expert	Master
Tuition/Day	40 G	50 G	60 G	70 G
Decorum	+1	+1 to 2	+1 to 3	+1 to 4
Refinement	+1	+1 to 2	+1 to 3	+1 to 4

    */
	{ "MannersClass",  // +name of the action
    { /*str*/0, /*con*/0, /*int*/0, /*ref*/1, /*cha*/0, /*mor*/0, /*fai*/0, /*sin*/0, /*sen*/0, // 9 stats changes
        /*cs*/0,  /*ca*/0,  /*cd*/0,  /*ms*/0, /*ma*/0, /*md*/0, /*dec*/1, /*art*/0, /*elo*/0, /*coo*/0, /*cle*/0, /*tem*/0 // 12 skills changes
        } },
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
    // Load the specimen into the engine - assume that inclinations are already set

    engine->getInputVariable(5)->setValue(std::get<0>(stats));
    engine->getInputVariable(6)->setValue(std::get<1>(stats));
    engine->getInputVariable(7)->setValue(std::get<2>(stats));
    engine->getInputVariable(8)->setValue(std::get<3>(stats));
    engine->getInputVariable(9)->setValue(std::get<4>(stats));
    engine->getInputVariable(10)->setValue(std::get<5>(stats));
    engine->getInputVariable(11)->setValue(std::get<6>(stats));
    engine->getInputVariable(12)->setValue(std::get<7>(stats));
    engine->getInputVariable(13)->setValue(std::get<8>(stats));
    engine->getInputVariable(14)->setValue(std::get<9>(stats));
    engine->getInputVariable(15)->setValue(std::get<10>(stats));
    engine->getInputVariable(16)->setValue(std::get<11>(stats));
    engine->getInputVariable(17)->setValue(std::get<12>(stats));
    engine->getInputVariable(18)->setValue(std::get<13>(stats));
    engine->getInputVariable(19)->setValue(std::get<14>(stats));
    engine->getInputVariable(20)->setValue(std::get<15>(stats));
    engine->getInputVariable(21)->setValue(std::get<16>(stats));
    engine->getInputVariable(22)->setValue(std::get<17>(stats));
    engine->getInputVariable(23)->setValue(std::get<18>(stats));
    engine->getInputVariable(24)->setValue(std::get<19>(stats));
    engine->getInputVariable(25)->setValue(std::get<20>(stats));

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
    std::string path{ "C:\\projects\\pm_solver\\Baseline.fll" };
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
    // Social reputation > 100
	const int social_reputation = std::get<3>(stats) + std::get<4>(stats) + std::get<5>(stats)
        + std::get<15>(stats) + std::get<16>(stats) + std::get<17>(stats);

    // Fighter reputation < 100
    const int fighter_reputation = std::get<0>(stats) + std::get<1>(stats)
		+ std::get<9>(stats) + std::get<10>(stats) + std::get<11>(stats);

	// the higher the social reputation than the 100 the lower the fitness value
	// the higher the fighter reputation than the 100 the higher the fitness value
	// social reputation > 100, fighter reputation = 0 => fitness = 0
    // social reputation < 100, fighter reputation = 0 => fitness > 0
    // social reputation > 100, fighter reputation > 0 => fitness > 0
	// social reputation < 100, fighter reputation > 0 => fitness >> 0
	
	double fitness_value = 0.0;
	
	// Penalty if social reputation is below 100
	if (social_reputation < 100) {
		fitness_value += (100 - social_reputation);
	}
	
	// Penalty if fighter reputation is above 0
	if (fighter_reputation > 0) {
		fitness_value += fighter_reputation;
	}
	
	return fitness_value;
}

constexpr int T = 120; // number of steps to take

std::pair<std::vector<std::string>, double> simulate(const Inclinations& inclinations, fl::Engine* engine)
{
    // Initialize a specimen
    Stats stats{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<std::string> path{};

    engine->restart();

    // Set inclinations
    engine->getInputVariable(0)->setValue(std::get<0>(inclinations));
    engine->getInputVariable(1)->setValue(std::get<1>(inclinations));
    engine->getInputVariable(2)->setValue(std::get<2>(inclinations));
    engine->getInputVariable(3)->setValue(std::get<3>(inclinations));
    engine->getInputVariable(4)->setValue(std::get<4>(inclinations));

    for (int i = 0; i < T; ++i)
    {
        std::cout << "Step " << i + 1 << ":\n";
        auto step = single_step(stats, inclinations, engine);

        print_stats(stats);

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


std::string choose_action_fast(fl::Engine* engine, const Stats& stats)
{
    // Load the specimen into the engine - assume that inclinations are already set

    engine->getInputVariable(5)->setValue(std::get<0>(stats));
    engine->getInputVariable(6)->setValue(std::get<1>(stats));
    engine->getInputVariable(7)->setValue(std::get<2>(stats));
    engine->getInputVariable(8)->setValue(std::get<3>(stats));
    engine->getInputVariable(9)->setValue(std::get<4>(stats));
    engine->getInputVariable(10)->setValue(std::get<5>(stats));
    engine->getInputVariable(11)->setValue(std::get<6>(stats));
    engine->getInputVariable(12)->setValue(std::get<7>(stats));
    engine->getInputVariable(13)->setValue(std::get<8>(stats));
    engine->getInputVariable(14)->setValue(std::get<9>(stats));
    engine->getInputVariable(15)->setValue(std::get<10>(stats));
    engine->getInputVariable(16)->setValue(std::get<11>(stats));
    engine->getInputVariable(17)->setValue(std::get<12>(stats));
    engine->getInputVariable(18)->setValue(std::get<13>(stats));
    engine->getInputVariable(19)->setValue(std::get<14>(stats));
    engine->getInputVariable(20)->setValue(std::get<15>(stats));
    engine->getInputVariable(21)->setValue(std::get<16>(stats));
    engine->getInputVariable(22)->setValue(std::get<17>(stats));
    engine->getInputVariable(23)->setValue(std::get<18>(stats));
    engine->getInputVariable(24)->setValue(std::get<19>(stats));
    engine->getInputVariable(25)->setValue(std::get<20>(stats));

    // Get action priorities
    engine->process();

    const auto& output_vars = engine->outputVariables();
    auto it = std::max_element(
        output_vars.begin(), output_vars.end(),
        [](const auto* a, const auto* b) {
            // defaulting to 0 if the value is NaN
            const auto left_priority = std::isnan(a->getValue()) ? 0.0 : a->getValue();
            const auto right_priority = std::isnan(b->getValue()) ? 0.0 : b->getValue();

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

void single_step_fast(Stats& stats, fl::Engine* engine)
{
    // Choose an action based on the current stats and inclinations
    std::string chosen_action_name = choose_action_fast(engine, stats);
    // Apply the effects of the chosen action
    Stats stats_diff = actions.at(chosen_action_name);
    stats = sum_stats(stats, stats_diff);
}

double simulate_fast(const Inclinations& inclinations, fl::Engine* engine)
{
    // Initialize a specimen
    Stats stats{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    engine->restart();

    // Set inclinations
    engine->getInputVariable(0)->setValue(std::get<0>(inclinations));
    engine->getInputVariable(1)->setValue(std::get<1>(inclinations));
    engine->getInputVariable(2)->setValue(std::get<2>(inclinations));
    engine->getInputVariable(3)->setValue(std::get<3>(inclinations));
    engine->getInputVariable(4)->setValue(std::get<4>(inclinations));

    for (int i = 0; i < T; ++i)
    {
        single_step_fast(stats, engine);
    }

    return fitness(stats);
}

static auto engine = init(); // this is super slow but fuzzylite is not prepared for multithreading so we need to create a new engine for each call

// Pagmo2-compatible problem definition
struct pm_problem {

    // Implementation of the objective function.
    pagmo::vector_double fitness(const pagmo::vector_double& dv) const
    {
        const Inclinations specimen{ dv[0], dv[1], dv[2], dv[3], dv[4] };

        std::unique_ptr<fl::Engine> engine_copy(engine.get()->clone()); // wrap in unique_ptr for automatic cleanup

        return { simulate_fast(specimen, engine_copy.get())};
    }

    /**
     * Implementation of the box bounds.
     * First element is the lower bound, second is the upper bound.
     * Bounds are inclination values for five our inclinations.
     * 
     * (Range is 0.0 - 1.0, we hope that Pagmo2 will correctly interpolate between them)
     */
    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const
    {
        return { {0., 0., 0., 0., 0.}, {1., 1., 1., 1., 1.} };
    }
};

void test_simulation(const Inclinations &specimen)
{
    std::unique_ptr<fl::Engine> engine_clone(engine.get()->clone());
    const auto& result = simulate(
        specimen,
        engine_clone.get()
    );
    print_simulation_result(result);
}

int main_bak()
{
    pagmo::problem prob(pm_problem{});

    pagmo::algorithm algo(pagmo::sade(100));

    pagmo::archipelago archi(16u, algo, prob, 20u);

    archi.evolve(10);

    archi.wait_check();

    pagmo::vector_double best_champion;
	double best_fitness = std::numeric_limits<double>::max();

    for (const auto& isl : archi)
    {
        const auto& champion = isl.get_population().champion_x();
        std::cout << "island champion: {" << champion[0] << ", " << champion[1] << ", " << champion[2] << ", " << champion[3] << ", " << champion[4] << "}\n";

        if (isl.get_population().champion_f()[0] < best_fitness)
        {
            best_fitness = isl.get_population().champion_f()[0];
            best_champion = champion;
        }
    }


    test_simulation({ best_champion[0], best_champion[1], best_champion[2], best_champion[3], best_champion[4] });

    return 0;
}


int main()
{
    std::string status;
    if (not engine->isReady(&status))
    {
        throw fl::Exception("[engine error] engine is not ready: \n" + status);
    }

    //// Example specimen with balanced inclinations
    //const Inclinations specimen{ 0.01, 0.2, 0.7, 0.9, 0.01 };
    //test_simulation(specimen);
    return 0;
}