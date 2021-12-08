/*
 * This module makes the function for generating random values
 * in pgbench available as a user-defined function.
 */

#include <math.h>
#include "postgres.h"
#include "fmgr.h"
#include "port.h"


#define MAX_ZIPFIAN_PARAM		1000.0
#define MIN_ZIPFIAN_PARAM		1.001

PG_MODULE_MAGIC;

/* SQL functions */
PG_FUNCTION_INFO_V1(random_exponential);
PG_FUNCTION_INFO_V1(random_gaussian);
PG_FUNCTION_INFO_V1(random_zipfian);

/* random state */
typedef struct RandomState
{
	unsigned short xseed[3];
} RandomState;

static RandomState base_random_sequence;

static int64 getExponentialRand(RandomState *random_state, int64 min, int64 max, double parameter);
static int64 getGaussianRand(RandomState *random_state, int64 min, int64 max, double parameter);
static int64 computeIterativeZipfian(RandomState *random_state, int64 n, double s);
static int64 getZipfianRand(RandomState *random_state, int64 min, int64 max, double s);

static void
initRandomState()
{
	uint64	  iseed;
	if (!pg_strong_random(&iseed, sizeof(iseed)))
		elog(ERROR, "could not generate random seed");

	/* Fill base_random_sequence with low-order bits of seed */
	base_random_sequence.xseed[0] = iseed & 0xFFFF;
	base_random_sequence.xseed[1] = (iseed >> 16) & 0xFFFF;
	base_random_sequence.xseed[2] = (iseed >> 32) & 0xFFFF;
}

/*
 * random number generator: exponential distribution from min to max inclusive.
 * the parameter is so that the density of probability for the last cut-off max
 * value is exp(-parameter).
 */
static int64
getExponentialRand(RandomState *random_state, int64 min, int64 max,
				   double parameter)
{
	double	  cut,
				uniform,
				rand;

	/* abort if wrong parameter, but must really be checked beforehand */
	Assert(parameter > 0.0);
	cut = exp(-parameter);
	/* erand in [0, 1), uniform in (0, 1] */
	uniform = 1.0 - pg_erand48(random_state->xseed);

	/*
	 * inner expression in (cut, 1] (if parameter > 0), rand in [0, 1)
	 */
	Assert((1.0 - cut) != 0.0);
	rand = -log(cut + (1.0 - cut) * uniform) / parameter;
	/* return int64 random number within between min and max */
	return min + (int64) ((max - min + 1) * rand);
}

/* random number generator: gaussian distribution from min to max inclusive */
static int64
getGaussianRand(RandomState *random_state, int64 min, int64 max,
				double parameter)
{
	double	  stdev;
	double	  rand;

	/* abort if parameter is too low, but must really be checked beforehand */
	Assert(parameter >= MIN_GAUSSIAN_PARAM);

	/*
	 * Get user specified random number from this loop, with -parameter <
	 * stdev <= parameter
	 *
	 * This loop is executed until the number is in the expected range.
	 *
	 * As the minimum parameter is 2.0, the probability of looping is low:
	 * sqrt(-2 ln(r)) <= 2 => r >= e^{-2} ~ 0.135, then when taking the
	 * average sinus multiplier as 2/pi, we have a 8.6% looping probability in
	 * the worst case. For a parameter value of 5.0, the looping probability
	 * is about e^{-5} * 2 / pi ~ 0.43%.
	 */
	do
	{
		/*
		 * pg_erand48 generates [0,1), but for the basic version of the
		 * Box-Muller transform the two uniformly distributed random numbers
		 * are expected in (0, 1] (see
		 * https://en.wikipedia.org/wiki/Box-Muller_transform)
		 */
		double	  rand1 = 1.0 - pg_erand48(random_state->xseed);
		double	  rand2 = 1.0 - pg_erand48(random_state->xseed);

		/* Box-Muller basic form transform */
		double	  var_sqrt = sqrt(-2.0 * log(rand1));

		stdev = var_sqrt * sin(2.0 * M_PI * rand2);

		/*
		 * we may try with cos, but there may be a bias induced if the
		 * previous value fails the test. To be on the safe side, let us try
		 * over.
		 */
	}
	while (stdev < -parameter || stdev >= parameter);

	/* stdev is in [-parameter, parameter), normalization to [0,1) */
	rand = (stdev + parameter) / (parameter * 2.0);

	/* return int64 random number within between min and max */
	return min + (int64) ((max - min + 1) * rand);
}


/*
 * Computing zipfian using rejection method, based on
 * "Non-Uniform Random Variate Generation",
 * Luc Devroye, p. 550-551, Springer 1986.
 *
 * This works for s > 1.0, but may perform badly for s very close to 1.0.
 */
static int64
computeIterativeZipfian(RandomState *random_state, int64 n, double s)
{
	double	  b = pow(2.0, s - 1.0);
	double	  x,
				t,
				u,
				v;

	/* Ensure n is sane */
	if (n <= 1)
		return 1;

	while (true)
	{
		/* random variates */
		u = pg_erand48(random_state->xseed);
		v = pg_erand48(random_state->xseed);

		x = floor(pow(u, -1.0 / (s - 1.0)));

		t = pow(1.0 + 1.0 / x, s - 1.0);
		/* reject if too large or out of bound */
		if (v * x * (t - 1.0) / (b - 1.0) <= t / b && x <= n)
			break;
	}
	return (int64) x;
}

/* random number generator: zipfian distribution from min to max inclusive */
static int64
getZipfianRand(RandomState *random_state, int64 min, int64 max, double s)
{
	int64	   n = max - min + 1;

	/* abort if parameter is invalid */
	Assert(MIN_ZIPFIAN_PARAM <= s && s <= MAX_ZIPFIAN_PARAM);

	return min - 1 + computeIterativeZipfian(random_state, n, s);
}

/*
 * random_exponential: 
 * Return a bigint value that is a random value of the exponential distribution.
 */
Datum
random_exponential(PG_FUNCTION_ARGS)
{
	int64 ub, lb;
	double param;

	lb = PG_GETARG_INT64(0);
	ub = PG_GETARG_INT64(1);
	param = PG_GETARG_FLOAT8(2);
	initRandomState();
	PG_RETURN_INT64(getExponentialRand(&base_random_sequence, lb, ub, param));
}

/*
 * random_gaussian: 
 * Return a bigint value that is a random value of the gaussian distribution.
 */
Datum
random_gaussian(PG_FUNCTION_ARGS)
{
	int64 ub, lb;
	double param;

	lb = PG_GETARG_INT64(0);
	ub = PG_GETARG_INT64(1);
	param = PG_GETARG_FLOAT8(2);
	initRandomState();
	PG_RETURN_INT64(getGaussianRand(&base_random_sequence, lb, ub, param));
}

/*
 * random_zipfian: 
 * Return a bigint value that is a random value of the zipfian distribution.
 */
Datum
random_zipfian(PG_FUNCTION_ARGS)
{
	int64 ub, lb;
	double param;

	lb = PG_GETARG_INT64(0);
	ub = PG_GETARG_INT64(1);
	param = PG_GETARG_FLOAT8(2);
	if (param < MIN_ZIPFIAN_PARAM || param > MAX_ZIPFIAN_PARAM)
	{
		elog(ERROR, "zipfian parameter must be in range [%.3f, %.0f] (not %f)",
			MIN_ZIPFIAN_PARAM, MAX_ZIPFIAN_PARAM, param);
	}
	initRandomState();
	PG_RETURN_INT64(getZipfianRand(&base_random_sequence, lb, ub, param));
}
