#ifndef GEOM_PARTICLE_PARTICLEFILTER_H
#define GEOM_PARTICLE_PARTICLEFILTER_H

#include "util/matrix.h"
#include "util/time.h"
#include "util/param.h"
#include <iostream>

extern DoubleParam PARTICLE_FILTER_STDDEV;
extern DoubleParam PARTICLE_FILTER_DECAYRATE;

/**
 * \brief Implements the basic mathematics of a Particle filter.
 */
class ParticleFilter {
	public:
		ParticleFilter(double length, double offset, unsigned int numPartitions);
		~ParticleFilter();

		void update(double timeDelta);

		void add(double value, unsigned int numParticles);

		double getEstimate();
		double getLength();
		double getOffset();
		int getNumParticles();
		void toString();

	private:

		void updateEstimatedPartition();
		void clearWeights(unsigned int startIndex, unsigned int endIndex);

		double velocity_;
		double accel_;
		double estimate_;
		int prevEstimate_;
		unsigned int numPartitions_;
		double length_;
		double offset_;
		bool estimateValid_;
		bool prevEstimateValid_;
		bool velocityValid_;
		unsigned int *weight_;
};

bool pairCompare(const std::pair<int, int>& a, const std::pair<int, int>& b);

#endif

