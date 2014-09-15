#include "ai/hl/hl.h"
#include "ai/hl/stp/world.h"
#include "util/dprint.h"
#include <cassert>
#include <cmath>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/scrollbar.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class SimPassPlayer : public HighLevel {
		public:
			explicit SimPassPlayer(World world) : world(world), adj_xposition(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI), hsb_xposition(adj_xposition), xpos(0.0), adj_yposition(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI), hsb_yposition(adj_yposition), ypos(0.0), adj_orientation(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI), hsb_orientation(adj_orientation), orient(Angle::zero()) {
				adj_orientation.signal_value_changed().connect(sigc::mem_fun(*this, &SimPassPlayer::on_orientation_value_changed));
				adj_xposition.signal_value_changed().connect(sigc::mem_fun(*this, &SimPassPlayer::on_xposition_value_changed));
				adj_yposition.signal_value_changed().connect(sigc::mem_fun(*this, &SimPassPlayer::on_yposition_value_changed));
				ui_box.add(hsb_orientation);
				ui_box.add(hsb_xposition);
				ui_box.add(hsb_yposition);
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return &ui_box;
			}

			void tick() {
				FriendlyTeam friendly = world.friendly_team();
				if (!friendly.size()) {
					return;
				} else if (friendly.size() >= 1) {
					Player receiver = friendly[0];
					Point receiver_pos = receiver.position();
					Point ball_pos = world.ball().position();
					Point ball_vel = world.ball().velocity();
					Angle theta = Angle::of_radians(-std::atan(ball_vel.y / ball_vel.x));
					Point right_intersect = Point(receiver_pos.rotate(theta).x, ball_pos.rotate(theta).y).rotate(-theta);
					receiver.move(right_intersect, Angle::half() - ball_vel.orientation(), Point());
					to_draw = right_intersect;
					to_draw_orient = Angle::half() - ball_vel.orientation();
				}
			}
			Gtk::VBox ui_box;
			Gtk::Adjustment adj_xposition;
			Gtk::HScrollbar hsb_xposition;
			double xpos;
			Gtk::Adjustment adj_yposition;
			Gtk::HScrollbar hsb_yposition;
			double ypos;
			Gtk::Adjustment adj_orientation;
			Gtk::HScrollbar hsb_orientation;
			Angle orient;


			Point to_draw;
			Angle to_draw_orient;
			Player fake_enemy;

			void on_orientation_value_changed() {
				orient = Angle::of_radians(hsb_orientation.get_value());
			}

			void on_xposition_value_changed() {
				xpos = hsb_xposition.get_value();
			}

			void on_yposition_value_changed() {
				ypos = hsb_yposition.get_value();
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				ctx->set_source_rgb(0.0, 1.0, 0.0);
				ctx->arc(to_draw.x, to_draw.y, 0.09, to_draw_orient.to_radians() - 0.2 * M_PI, to_draw_orient.to_radians() + 0.2 * M_PI);
			}
	};
}

HIGH_LEVEL_REGISTER(SimPassPlayer)

