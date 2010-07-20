#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"
#include "ai/util.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include <gtkmm.h>

namespace {

	class HandTuning;

	/** Enables parameter tuning by hand.
	 */
	class HandTuningUI : public Gtk::Window {
		public:
			HandTuningUI(HandTuning* h) : ht(h) {
				set_title("Parameters");
				//vbox.add(button);
				//button.signal_clicked().connect(sigc::mem_fun(this, &HandTuningUI::run));
				//button.set_label("Run!");
				//add(vbox);
				add(table);
			}

			~HandTuningUI() {
				for (size_t i = 0; i < entries.size(); ++i) {
					table.remove(*entries[i]);
					table.remove(*labels[i]);
					delete entries[i];
					delete labels[i];
				}
			}

			// void run();

			/// Resets the parameters on the ui.
			void reset(TunableController* tc) {
				for (size_t i = 0; i < entries.size(); ++i) {
					table.remove(*entries[i]);
					table.remove(*labels[i]);
					delete entries[i];
					delete labels[i];
				}
				if (tc == NULL) {
					entries.clear();
					labels.clear();
					hide_all();
					return;
				}
				size_t P = tc->get_params().size();
				const std::vector<std::string>& names = tc->get_params_name();
				const std::vector<double>& vals = tc->get_params();
				labels.resize(P);
				entries.resize(P);
				table.resize(P, 2);
				for (size_t i = 0; i < P; ++i) {
					labels[i] = new Gtk::Label(names[i]);
					entries[i] = new Gtk::Entry();
					table.attach(*labels[i], 0, 1, i, i+1);
					table.attach(*entries[i], 1, 2, i, i+1);
					Glib::ustring str = Glib::ustring::format(std::fixed, std::setprecision(3), vals[i]);
					entries[i]->set_text(str);
				}
				show_all();
			}

			const std::vector<double> read_params() const {
				std::vector<double> ret(entries.size(), 0);
				for (size_t i = 0; i < entries.size(); ++i) {
					Glib::ustring str = entries[i]->get_text();
					ret[i] = atof(str.c_str());
				}
				return ret;
			}

		protected:
			bool on_delete_event(GdkEventAny *) {
				Gtk::Main::quit();
				return true;
			}

		private:
			int params;
			Gtk::Table table;
			// Gtk::VBox vbox;
			// Gtk::Button button;
			std::vector<Gtk::Label*> labels;
			std::vector<Gtk::Entry*> entries;
			HandTuning* ht;
	};

	class HandTuning : public MovementBenchmark {
		public:
			HandTuning(RefPtr<World>);
			~HandTuning();
			Gtk::Widget *get_ui_controls();
			StrategyFactory &get_factory();
			void tick();
			void run();
			void stop();
			void reset();
		private:
			HandTuningUI ui;
			TunableController* tc;
			Gtk::Button run_button;
			Gtk::Button stop_button;
			Gtk::HBox hbox1;
			Gtk::CheckButton dribble_checkbutton;
			Gtk::HScale dribble_scale1;
			Gtk::HScale dribble_scale2;
			Gtk::Label dribble_label1;
			Gtk::Label dribble_label2;
			Gtk::Label dribble_text1;
			Gtk::Label dribble_text2;
			Gtk::VBox vbox;
	};

	HandTuning::HandTuning(RefPtr<World> world) : MovementBenchmark(world), ui(this), tc(NULL), run_button("Run"), stop_button("Stop"), dribble_checkbutton("Dribble"),dribble_scale1(0.0, 1.0, 0.01), dribble_scale2(0.0, 1.0, 0.01), dribble_label1("dribbling speed w/o ball"), dribble_label2("dribbling speed w ball") {
		run_button.signal_clicked().connect(sigc::mem_fun(this,&HandTuning::run));
		stop_button.signal_clicked().connect(sigc::mem_fun(this,&HandTuning::stop));
		done = tasks.size();
		time_steps = 0;
		hbox1.add(run_button);
		hbox1.add(stop_button);
		vbox.add(hbox1);
		vbox.add(dribble_checkbutton);
		vbox.add(dribble_label1);
		vbox.add(dribble_scale1);
		vbox.add(dribble_label2);
		vbox.add(dribble_scale2);
		vbox.add(dribble_text1);
		vbox.add(dribble_text2);
	}

	HandTuning::~HandTuning() {
	}

	Gtk::Widget *HandTuning::get_ui_controls() {
		return &vbox;
	}

	void HandTuning::reset() {
		tc = TunableController::get_instance();
		time_steps = 0;
		done = tasks.size();
		ui.reset(tc);
	}

	void HandTuning::run() {
		if (tc) {
			const std::vector<double>& params = ui.read_params();
			tc->set_params(params);
		}
		done = 0;
		time_steps = 0;
	}

	void HandTuning::stop() {
		done = tasks.size();
	}

	void HandTuning::tick() {
		const FriendlyTeam &the_team(the_world->friendly);
		if (the_team.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}
		if (tc != TunableController::get_instance()) {
			reset();
		}
		const RefPtr<Player> the_player = the_team.get_player(0);
		if (!AIUtil::has_ball(the_world, the_player)) {
			the_player->dribble(dribble_scale1.get_value());
		} else {
			the_player->dribble(dribble_scale2.get_value());
		}
		const Glib::ustring text1 = Glib::ustring::compose("Theory dribble %1", the_player->theory_dribbler_speed());
		const Glib::ustring text2 = Glib::ustring::compose("Actual dribble %1", the_player->dribbler_speed());
		dribble_text1.set_label(text1);
		dribble_text2.set_label(text2);
		if (dribble_checkbutton.get_active() && !AIUtil::has_ball(the_world, the_player)) {
			const Point balldist = the_world->ball()->position() - the_player->position();
			the_player->move(the_world->ball()->position(), atan2(balldist.y, balldist.x));
		} else {
			MovementBenchmark::tick();
		}
	}

	class HandTuningFactory : public StrategyFactory {
		public:
			HandTuningFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	HandTuningFactory::HandTuningFactory() : StrategyFactory("Hand Tune & Move Benchmark") {
	}

	RefPtr<Strategy2> HandTuningFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new HandTuning(world));
		return s;
	}

	HandTuningFactory factory;

	StrategyFactory &HandTuning::get_factory() {
		return factory;
	}

}
