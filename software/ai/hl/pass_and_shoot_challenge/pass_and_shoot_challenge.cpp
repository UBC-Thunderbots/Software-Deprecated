#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/intercept.h"
#include "geom/util.h"
#include<iostream>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const unsigned int min_team_size = 4;

	Point bot0_initial(-2.75,-1.2);
	Point bot1_initial(-2.00,1.2);
	Point bot2_initial(-1.00,-1.2);
	Point bot3_initial(0.00,1.2);
	Angle robot0_orientation = (bot1_initial - bot0_initial).orientation();
	Angle robot1_orientation = (bot0_initial - bot1_initial).orientation();
	Angle robot2_orientation = (bot1_initial - bot2_initial).orientation();
	Angle robot3_orientation = (bot2_initial - bot3_initial).orientation();
	double kick_speed = 0.05;

	enum state{state_zero, state_one, state_two, state_three, state_four, state_five, state_six, state_seven};

};


class PASCHL : public HighLevel {
		public:
			PASCHL(World &world) : world(world) {
				robot_positions.push_back(std::make_pair(bot0_initial, robot0_orientation));
				robot_positions.push_back(std::make_pair(bot1_initial,robot1_orientation));
				robot_positions.push_back(std::make_pair(bot2_initial,robot2_orientation));
				robot_positions.push_back(std::make_pair(bot3_initial,robot3_orientation));

				current_state = state_zero;

			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();
				Player::Ptr player1 = friendly.get(1);

					if(friendly.size() < min_team_size) {
						return;
					}



				switch(current_state) {
				case state_zero:
					for(unsigned int i = 0 ;i < min_team_size; i++) {
						friendly.get(i)->move(robot_positions[i].first, robot_positions[i].second, Point());
					}

					current_state=state_one;
					break;
				case state_one:{
					Point intercept_location = horizontal_intercept(player1);
					bool kicked_ball = false;
					if(friendly.get(0)->has_ball()) {
						AI::HL::STP::Action::autokick(friendly.get(0), Point(), kick_speed);
						kicked_ball = true;
					}
					if(!((intercept_location - Point(0,0)).len() < 1e-9) && (std::fabs(intercept_location.y-player1->position().y) < 1e-9) && kicked_ball) {
						player1->move(Point(intercept_location.x, player1->position().y), robot_positions[1].second, Point());
					}

					std::cout<<intercept_location<<std::endl;
					if(friendly.get(1)->has_ball())
						current_state = state_two;
				}
					break;
				case state_two:
					std::cout<<"hello"<<std::endl;
					break;
				case state_three:
					break;

				case state_four:
					break;
				case state_five:
					break;
				case state_six:
					break;
				case state_seven:
					break;
				}

			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
			std::vector<std::pair<Point, Angle>> robot_positions;
			state current_state;
			Point horizontal_intercept(Player::Ptr player);


};

Point PASCHL::horizontal_intercept(Player::Ptr player){
	double top_horizontal_line=1.2;
	double width_of_rectangle = 2;
	double height_of_rectangle = top_horizontal_line*2;
	Rect ball_intercept_boundary(Point((player->position().x - width_of_rectangle*.5), -top_horizontal_line), height_of_rectangle, width_of_rectangle);

	return vector_rect_intersect(ball_intercept_boundary,world.ball().position(), world.ball().velocity()+ world.ball().position());
}


HIGH_LEVEL_REGISTER(PASCHL)

