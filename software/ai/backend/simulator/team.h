#ifndef AI_BACKEND_SIMULATOR_TEAM_H
#define AI_BACKEND_SIMULATOR_TEAM_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/robot.h"
#include "simulator/sockproto/proto.h"
#include "util/box_array.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>

namespace AI {
	namespace BE {
		namespace Simulator {
			class Backend;

			/**
			 * \brief A general simulator-based team, whether friendly or enemy.
			 *
			 * \tparam T the type of robot held in this team.
			 */
			template<typename T> class GenericTeam : public NonCopyable {
				public:
					/**
					 * \brief The property holding the team's score.
					 */
					Property<unsigned int> score_prop;

					/**
					 * \brief Constructs a new GenericTeam.
					 */
					GenericTeam() : score_prop(0) {
					}

					/**
					 * \brief Creates a robot.
					 *
					 * \param[in] be the backend to attach the robot to.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					void create(Backend &be, unsigned int pattern) {
						assert(pattern < members.SIZE);
						members.create(pattern, std::ref(be), pattern);
						populate_pointers();
						emit_membership_changed();
					}

					/**
					 * \brief Destroys a robot.
					 *
					 * \param[in] pattern the pattern index to destroy.
					 */
					void destroy(unsigned int pattern) {
						assert(pattern < members.SIZE);
						members.destroy(pattern);
						populate_pointers();
						emit_membership_changed();
					}

					/**
					 * \brief Emits the membership-changed signal.
					 */
					virtual void emit_membership_changed() const = 0;

					unsigned int score() const { return score_prop; }
					std::size_t size() const { return member_ptrs.size(); }
					typename T::Ptr get(std::size_t i) { return member_ptrs[i]; }
					typename T::CPtr get(std::size_t i) const { return member_ptrs[i]; }

				private:
					/**
					 * \brief The members of the team.
					 */
					BoxArray<T, 16> members;

					std::vector<BoxPtr<T>> member_ptrs;

					/**
					 * \brief Rebuilds the member_ptrs array.
					 */
					void populate_pointers() {
						member_ptrs.clear();
						for (std::size_t i = 0; i < members.SIZE; ++i) {
							const BoxPtr<T> &p = members[i];
							if (p) {
								member_ptrs.push_back(p);
							}
						}
					}
			};

			/**
			 * The team containing \ref Player "Players" that the AI can control.
			 */
			class FriendlyTeam : public AI::BE::FriendlyTeam, public GenericTeam<Player> {
				public:
					/**
					 * Constructs a new FriendlyTeam.
					 *
					 * \param[in] be the backend under which the team lives.
					 */
					FriendlyTeam(Backend &be) : be(be) {
					}

					/**
					 * Retrieves a Player from the team.
					 *
					 * \param[in] i the index of the Player to retrieve.
					 *
					 * \return the Player.
					 */
					Player::Ptr get_impl(std::size_t i) { return GenericTeam<Player>::get(i); }

					/**
					 * Retrieves a Player from the team.
					 *
					 * \param[in] i the index of the Player to retrieve.
					 *
					 * \return the Player.
					 */
					Player::CPtr get_impl(std::size_t i) const { return GenericTeam<Player>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo(&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const timespec &ts) {
						// Record new score.
						score_prop = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].robot_info.pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								destroy(get(i)->pattern());
							}
						}

						// Add any newly-created robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (state[i].robot_info.pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									create(be, state[i].robot_info.pattern);
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									Player::Ptr plr = get_impl(j);
									if (state[i].robot_info.pattern == plr->pattern()) {
										plr->pre_tick(state[i], ts);
									}
								}
							}
						}
					}

					/**
					 * Stores the orders computed by the AI into a packet to send to the simulator.
					 *
					 * \param[out] orders the packet to populate.
					 */
					void encode_orders(::Simulator::Proto::A2SPlayerInfo(&orders)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM]) {
						for (std::size_t i = 0; i < G_N_ELEMENTS(orders); ++i) {
							if (i < size()) {
								get_impl(i)->encode_orders(orders[i]);
							} else {
								orders[i].pattern = std::numeric_limits<unsigned int>::max();
							}
						}
					}

					unsigned int score() const { return GenericTeam<Player>::score(); }
					std::size_t size() const { return GenericTeam<Player>::size(); }
					AI::BE::Player::Ptr get(std::size_t i) { return get_impl(i); }
					AI::BE::Player::CPtr get(std::size_t i) const { return get_impl(i); }
					void emit_membership_changed() const { signal_membership_changed().emit(); }

				private:
					/**
					 * \brief The backend under which the team lives.
					 */
					Backend &be;
			};

			/**
			 * The team containing \ref Robot "Robots" that are controlled by another AI.
			 */
			class EnemyTeam : public AI::BE::EnemyTeam, public GenericTeam<Robot> {
				public:
					/**
					 * Constructs a new EnemyTeam.
					 *
					 * \param[in] be the backend under which the team lives.
					 */
					EnemyTeam(Backend &be) : be(be) {
					}

					/**
					 * Retrieves a Robot from the team.
					 *
					 * \param[in] i the index of the Robot to retrieve.
					 *
					 * \return the Robot.
					 */
					Robot::Ptr get_impl(std::size_t i) { return GenericTeam<Robot>::get(i); }

					/**
					 * Retrieves a Robot from the team.
					 *
					 * \param[in] i the index of the Robot to retrieve.
					 *
					 * \return the Robot.
					 */
					Robot::CPtr get_impl(std::size_t i) const { return GenericTeam<Robot>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ARobotInfo(&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const timespec &ts) {
						// Record new score.
						score_prop = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								destroy(get(i)->pattern());
							}
						}

						// Add any newly-created robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (state[i].pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									create(be, state[i].pattern);
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									Robot::Ptr bot = get_impl(j);
									if (state[i].pattern == bot->pattern()) {
										bot->pre_tick(state[i], ts);
									}
								}
							}
						}
					}

					unsigned int score() const { return GenericTeam<Robot>::score(); }
					std::size_t size() const { return GenericTeam<Robot>::size(); }
					AI::BE::Robot::Ptr get(std::size_t i) { return get_impl(i); }
					AI::BE::Robot::CPtr get(std::size_t i) const { return get_impl(i); }
					void emit_membership_changed() const { signal_membership_changed().emit(); }

				private:
					/**
					 * \brief The backend under which the team lives.
					 */
					Backend &be;
			};
		}
	}
}

#endif

