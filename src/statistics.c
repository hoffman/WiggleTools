// Copyright 2013 EMBL-EBI
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <math.h>
#include <stdlib.h>
#include <string.h>

// Local header
#include "wiggleIterator.h"
#include "multiplexer.h"

//////////////////////////////////////////////////////
// Basic Stats
//////////////////////////////////////////////////////

double AUC(WiggleIterator * wi) {
	double total = 0;
	wi = NonOverlappingWiggleIterator(wi);
	for(;!wi->done; pop(wi)) 
		total += (wi->finish - wi->start) * wi->value;
	return total;
}

double span(WiggleIterator * wi) {
	double total = 0;
	wi = NonOverlappingWiggleIterator(wi);
	for(;!wi->done; pop(wi)) 
		if (wi->value)
			total += (wi->finish - wi->start);
	return total;
}

double mean(WiggleIterator * wi) {
	double total = 0;
	double span = 0;
	wi = NonOverlappingWiggleIterator(wi);
	for(;!wi->done; pop(wi)) {
		span += (wi->finish - wi->start);
		total += (wi->finish - wi->start) * wi->value;
	}
	return total / span;
}

double variance(WiggleIterator * wi) {
	// Online algorithm copied from 
	// http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Two-pass_algorithm
	double sumWeight = 0;
	double mean = 0;
	double M2 = 0;
	double count = 0;
	wi = NonOverlappingWiggleIterator(wi);

	for(;!wi->done; pop(wi)) {
		double weight = wi->finish - wi->start;
		double temp = sumWeight + weight;
		double delta = wi->value - mean;
		double R = delta * weight / temp;
		mean += R;
		M2 += sumWeight * delta * R;
		sumWeight = temp;
		count++;
	}
	return (M2 * count) / (sumWeight * (count - 1));
}

double stddev(WiggleIterator * wi) {
	wi = NonOverlappingWiggleIterator(wi);
	return sqrt(variance(wi));
}

double isZero(WiggleIterator * wi) {
	wi = NonOverlappingWiggleIterator(wi);
	for (; !wi->done; pop(wi))
		if (wi->value != 0)
			exit(1);
	return 1;
}

//////////////////////////////////////////////////////
// Correlation function
//////////////////////////////////////////////////////
// Note: this is an approximate calculation of the Pearson calculation
// which has the benefit of running in a single pass through the data
// The origin of the code can be found at:
// http://en.wikipedia.org/wiki?title=Talk:Correlation

double pearsonCorrelation(WiggleIterator * iterA, WiggleIterator * iterB) {
	double sum_sq_A = 0;
	double sum_sq_B = 0;
	double sum_AB = 0;
	double meanA = iterA->value;
	double meanB = iterB->value;
	int totalLength = 0;
	int halfway, width;
	double sweep, deltaA, deltaB;
	WiggleIterator * iters[2];
	iters[0] = NonOverlappingWiggleIterator(iterA);
	iters[1] = NonOverlappingWiggleIterator(iterB);
	Multiplexer * multi;

	for (multi=newMultiplexer(iters, 2); !multi->done; popMultiplexer(multi)) {
		width = (multi->finish - multi->start);
		halfway = totalLength + width/2;
		if (halfway == 0)
			halfway = 1;
		sweep = (halfway - 1.0) / halfway;
		totalLength += width;
		if (multi->inplay[0])
			deltaA = multi->values[0] - meanA;
		else
			deltaA = -meanA;
		if (multi->inplay[1])
			deltaB = multi->values[1] - meanB;
		else
			deltaB = -meanB;
		sum_sq_A += deltaA * deltaA * sweep;
		sum_sq_B += deltaB * deltaB * sweep;
		sum_AB += deltaA * deltaB * sweep;
		meanA += deltaA / halfway;
		meanB += deltaB / halfway;
	}

	return sum_AB / (sqrt(sum_sq_A) * sqrt(sum_sq_B));
}
