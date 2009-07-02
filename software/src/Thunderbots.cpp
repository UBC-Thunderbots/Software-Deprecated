#include "AI/AITeam.h"
#include "AI/Simulator.h"
#include "AI/Visualizer.h"
#include "datapool/Config.h"
#include "datapool/HWRunSwitch.h"
#include "datapool/IntervalTimer.h"
#include "datapool/Noncopyable.h"
#include "datapool/RefBox.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "IR/ImageRecognition.h"
#include "Log/Log.h"
#include "UI/ControlPanel.h"
#include "XBee/XBeeBot.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <gtkmm.h>
#include <getopt.h>

namespace {
	void usage(const char *app) {
		std::cerr <<
			"Usage:\n"
			<< app << " options\n"
			"Options are:\n"
			"  -s\n"
			"  --simulator\n"
			"    Turns on the simulator. By default, image recognition and XBee are used.\n"
			"\n"
			"  -v\n"
			"  --visualizer\n"
			"    Turns on the visualizer. By default, no visualization is shown.\n"
			"\n"
			"  -r\n"
			"  --replay\n"
			"    Runs a replay of a match in the log instead of running a real match.\n"
			"\n"
			"  -d\n"
			"  --debug\n"
			"    Enables debugging output. By default, only informational, warning, and error messages are output.\n"
			"\n"
			"  -e\n"
			"  --enemy-ai\n"
			"    Enables AI control of the enemy team. By default, this only occurs in the simulator.\n";
	}

	class AIUpdater : public virtual sigc::trackable, private virtual Noncopyable {
	public:
		AIUpdater(const std::vector<Updateable *> &updateables) : updateables(updateables), timer(71428571ULL) {
			timer.signal_expire().connect(sigc::mem_fun(*this, &AIUpdater::onExpire));
		}

	private:
		const std::vector<Updateable *> &updateables;
		IntervalTimer timer;

		void onExpire() {
			for (unsigned int i = 0; i < updateables.size(); i++)
				updateables[i]->update();
		}
	};

	class PositionLogger : public virtual Noncopyable, public Updateable {
	public:
		PositionLogger() : lastSeenPlayType(PlayType::noType), lastSeenFriendlyScore(0), lastSeenEnemyScore(0), inited(false) {
			ofs.exceptions(std::ios_base::eofbit | std::ios_base::badbit | std::ios_base::failbit);

			const std::string &homeDir = Glib::getenv("HOME");
			if (homeDir == "") {
				Log::log(Log::LEVEL_ERROR, "XBee") << "Environment variable $HOME is not set!\n";
				std::exit(1);
			}

			const std::string &logFile = homeDir + "/.thunderbots/match.log";
			ofs.open(logFile.c_str(), std::ios_base::app | std::ios_base::out);
			ofs.precision(10);
		}

		void update() {
			if (!inited) {
				ofs << "\nI 1 " << std::time(0) << '\n';

				ofs << 'R';
				for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
					ofs << ' ' << World::get().player(i)->radius();
				ofs << ' ' << World::get().ball().radius() << '\n';

				inited = true;
			}

			const Field &field = World::get().field();
			if (field != lastSeenField) {
				ofs << "F " << field << '\n';
				lastSeenField = field;
			}

			if (World::get().playType() != lastSeenPlayType) {
				ofs << "P " << World::get().playType();
				lastSeenPlayType = World::get().playType();
				ofs << '\n';
			}

			if (World::get().friendlyTeam().score() != lastSeenFriendlyScore || World::get().enemyTeam().score() != lastSeenEnemyScore) {
				ofs << "S " << World::get().friendlyTeam().score() << ' ' << World::get().enemyTeam().score() << '\n';
				lastSeenFriendlyScore = World::get().friendlyTeam().score();
				lastSeenEnemyScore = World::get().enemyTeam().score();
			}

			ofs << "D";
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
				PPlayer pl = World::get().player(i);
				ofs << ' ' << pl->position().x << ' ' << pl->position().y << ' ' << pl->orientation();
			}
			ofs << ' ' << World::get().ball().position().x << ' ' << World::get().ball().position().y << '\n';
		}

	private:
		Field lastSeenField;
		PlayType::Type lastSeenPlayType;
		unsigned int lastSeenFriendlyScore, lastSeenEnemyScore;
		std::ofstream ofs;
		bool inited;
	};

	void execute(std::vector<Updateable *> &updateables) {
		updateables.insert(updateables.begin(), &World::get());
		for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
			updateables.push_back(World::get().player(i).get());
		updateables.push_back(&World::get().ball());
		PositionLogger plg;
		updateables.push_back(&plg);
		AIUpdater updater(updateables);
		Gtk::Main::run();
	}

	void execute(bool useVis, std::vector<Updateable *> &updateables) {
		if (useVis) {
			Visualizer vis;
			updateables.push_back(&vis);
			execute(updateables);
		} else {
			execute(updateables);
		}
	}
	
	void execute(bool useSim, bool useVis, Team &friendly, Team &enemy) {
		// Set team play sides.
		const std::string &val = Config::instance().getString("Game", "FriendlySide");
		if (val == "East") {
			friendly.side(false);
			enemy.side(true);
		} else if (val == "West") {
			friendly.side(true);
			enemy.side(false);
		} else {
			Log::log(Log::LEVEL_ERROR, "Main") << "Configuration file directive [Game]/FriendlySide must be either \"West\" or \"East\".\n";
			std::exit(1);
		}

		// Create world-interaction objects depending on simulation-vs-real-world mode.
		std::vector<Updateable *> updateables;
		if (useSim) {
			Simulator sim(friendly, enemy);
			updateables.push_back(&sim);
			RefBox refBox;
			execute(useVis, updateables);
		} else {
			XBeeBotSet xbee;
			HWRunSwitch runSwitch;
			ControlPanel cp;
			ImageRecognition ir(friendly, enemy);
			RefBox refBox;
			execute(useVis, updateables);
		}
	}

	void execute(bool useSim, bool useVis, bool useEnemyAI) {
		// Create shared object.
		Config config;

		// Create teams, depending on whether the enemy should be AI or not.
		AITeam friendly(0);
		if (useEnemyAI) {
			AITeam enemy(1);
			execute(useSim, useVis, friendly, enemy);
		} else {
			Team enemy(1);
			execute(useSim, useVis, friendly, enemy);
		}
	}

	class Replayer : public Updateable {
	public:
		Replayer() : friendly(0), enemy(1) {
			const std::string &homeDir = Glib::getenv("HOME");
			if (homeDir == "") {
				Log::log(Log::LEVEL_ERROR, "Replayer") << "Environment variable $HOME is not set!\n";
				std::exit(1);
			}

			const std::string &logFile = homeDir + "/.thunderbots/match.log";
			ifs.open(logFile.c_str(), std::ios_base::in);

			unsigned int recnum = 1;
			for (;;) {
				char ch;
				ifs >> ch;
				if (!ifs) {
					Log::log(Log::LEVEL_INFO, "Replayer") << "Reached end of log file at record " << recnum << '\n';
					break;
				}
				if (ch == 'I') {
					unsigned int version;
					ifs >> version;
					if (version != 1) {
						Log::log(Log::LEVEL_ERROR, "Replayer") << "Match data is in version " << version << " (not known)!\n";
						std::exit(1);
					}
					std::time_t timestamp;
					ifs >> timestamp;
					index.push_back(std::make_pair(timestamp, ifs.tellg()));
				} else if (ch == 'F') {
					Field f;
					ifs >> f;
				} else if (ch == 'D') {
					double db;
					for (unsigned int i = 0; i < 2 * Team::SIZE * 3 + 2; i++)
						ifs >> db;
				} else if (ch == 'R') {
					double db;
					for (unsigned int i = 0; i < 2 * Team::SIZE + 1; i++)
						ifs >> db;
				} else if (ch == 'P') {
					unsigned int i;
					ifs >> i;
				} else if (ch == 'S') {
					unsigned int i;
					ifs >> i;
					ifs >> i;
				} else {
					Log::log(Log::LEVEL_ERROR, "Replayer") << "Unrecognized record type " << ch << " at position " << recnum << '\n';
					std::exit(1);
				}
				recnum++;
			}

			Field field;
			World::init(friendly, enemy, field);
		}

		const std::vector<std::pair<std::time_t, std::streampos> > matches() const {
			return index;
		}

		void choose(unsigned int id) {
			assert(id <= index.size());
			ifs.clear();
			ifs.seekg(index[id].second);
			update();
			update();
		}

		void update() {
			std::string s;
			ifs >> s;
			char ch = s[0];
			if (!ifs) {
				Log::log(Log::LEVEL_INFO, "Replayer") << "Reached EOF in log file.\n";
				std::exit(0);
			}
			if (ch == 'I') {
				Log::log(Log::LEVEL_INFO, "Replayer") << "Reached end of match in log file.\n";
				std::exit(0);
			} else if (ch == 'F') {
				ifs >> World::get().field();
			} else if (ch == 'D') {
				for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
					PPlayer plr = World::get().player(i);
					double x, y, t;
					ifs >> x >> y >> t;
					plr->position(Vector2(x, y));
					plr->orientation(t);
				}
				double x, y;
				ifs >> x >> y;
				World::get().ball().position(Vector2(x, y));
			} else if (ch == 'R') {
				for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
					double r;
					ifs >> r;
					World::get().player(i)->radius(r);
				}
				double r;
				ifs >> r;
				World::get().ball().radius(r);
			} else if (ch == 'P') {
				unsigned int i;
				ifs >> i;
				PlayType::Type t = static_cast<PlayType::Type>(i);
				switch (t) {
					case PlayType::doNothing: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: doNothing\n"; break;
					case PlayType::start: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: start\n"; break;
					case PlayType::stop: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: stop\n"; break;
					case PlayType::play: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: play\n"; break;
					case PlayType::directFreeKick: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: directFreeKick\n"; break;
					case PlayType::indirectFreeKick: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: indirectFreeKick\n"; break;
					case PlayType::kickoff: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: kickoff\n"; break;
					case PlayType::preparePenaltyKick: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: preparePenaltyKick\n"; break;
					case PlayType::penaltyKick: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: penaltyKick\n"; break;
					case PlayType::penaltyKickoff: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: penaltyKickoff\n"; break;
					case PlayType::victoryDance: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: victoryDance\n"; break;
					case PlayType::prepareKickoff: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: prepareKickoff\n"; break;
					case PlayType::noType: Log::log(Log::LEVEL_ERROR, "Replayer") << "Play type: noType\n"; break;
				}
			} else if (ch == 'S') {
				unsigned int i;
				ifs >> i;
				World::get().team(0).score(i);
				ifs >> i;
				World::get().team(1).score(i);
			} else {
				Log::log(Log::LEVEL_ERROR, "Replayer") << "Unrecognized record type " << s << '\n';
				std::exit(1);
			}
		}

	private:
		std::ifstream ifs;
		std::vector<std::pair<std::time_t, std::streampos> > index;
		Team friendly, enemy;
	};

	void execute(bool useSim, bool useReplay, bool useVis, bool useEnemyAI) {
		if (useReplay) {
			Replayer repl;
			for (unsigned int i = 0; i < repl.matches().size(); i++) {
				struct tm *t = std::localtime(&repl.matches()[i].first);
				std::cerr << '[' << i << "] " << (1900 + t->tm_year) << '-' << t->tm_mon << '-' << t->tm_mday << ' ' << t->tm_hour << ':' << t->tm_min << ':' << t->tm_sec << '\n';
			}
			std::cerr << "Choose which match: ";
			unsigned int idx;
			std::cin >> idx;
			if (idx >= repl.matches().size()) {
				Log::log(Log::LEVEL_ERROR, "Replayer") << "Illegal match index chosen.\n";
				std::exit(1);
			}
			Log::log(Log::LEVEL_INFO, "Replayer") << "Playing match " << idx << '\n';
			repl.choose(idx);
			std::vector<Updateable *> upd;
			upd.push_back(&repl);
			Visualizer vis;
			upd.push_back(&vis);
			AIUpdater aiu(upd);
			Gtk::Main::run();
		} else {
			execute(useSim, useVis, useEnemyAI);
		}
	}
}

int main(int argc, char **argv) {
	// Create GTK main object.
	Gtk::Main kit(argc, argv);

	// Read remaining options.
	bool useSim = false;
	bool useVis = false;
	bool useReplay = false;
	bool useEnemyAI = false;
	static const option longopts[] = {
		{"simulator", 0, 0, 's'},
		{"visualizer", 0, 0, 'v'},
		{"replay", 0, 0, 'r'},
		{"debug", 0, 0, 'd'},
		{"enemy-ai", 0, 0, 'e'},
		{0, 0, 0, 0}
	};
	static const char shortopts[] = "svrde";
	int ch;
	while ((ch = getopt_long(argc, argv, shortopts, longopts, 0)) != -1) {
		switch (ch) {
			case 's':
				useSim = true;
				useEnemyAI = true;
				break;

			case 'v':
				useVis = true;
				break;

			case 'r':
				useSim = false;
				useReplay = true;
				useEnemyAI = false;
				useVis = true;
				break;

			case 'd':
				Log::setLevel(Log::LEVEL_DEBUG);
				break;

			case 'e':
				useEnemyAI = true;
				break;

			case '?':
				usage(argv[0]);
				return 1;
		}
	}

	// Run!
	execute(useSim, useReplay, useVis, useEnemyAI);

	return 0;
}

