/*
	Geiger-Mueller counter simulator.
	(c)2016-2018 Thomas Orgis <thomas@orgis.org>, licensed MIT-style.
	Essentially: This is just a bit of code that you can use as you please.

	This generates random pulses with a configured likelihood (event
	activity) that drive a simulated speaker to emit the characteristic
	ticking sound (as opposed to a perfect digital click).

	This is adapted from a stand-alone demonstration program to be
	directly available to out123 and others via libsyn123.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

/*
	Brain dump for the speaker model:

	Simulating a simple speaker as dampened oscillator.
	This should result in somewhat interesting impulse response.
	I do not bother with a lower frequency bound (people may wish to
	apply a highpass filter to simulate a small tinny speaker).

		F = - k x

	There is a mass attached, carrying the momentum. A new Geiger
	impulse just adds a big load of momentum and the oscillator
	works with that. A simple Euler scheme should be enough here.

		dx = v dt
		dv = F/m dt

	We need some friction here. Let's use simple quadratic (air)
	friction with coefficient r, (r v^2) being a force opposing the
	current movement.

		dv = - (k x + sign(v) r v^2) / m dt

	About the impulse from the counter ... it needs to be incorporated
	as an additional force G that is applied over a certain time period.
	I need an impulse shape. How does the cascade look again?
	Does a gauss curve do the trick? Or a jump with linear decay? Or simply
	a box impulse? I'll try with a box impulse, relating to a certain
	energy of a gas discharge. then, there's a dead time interval where
	new particles are not detected at all. After that, a time period where
	new discharges are weaker (linear ramp as simplest model).

	Let's use that model and solve it in the simplest manner:

		dx = v dt
		dv = (G(t) - k x(t) - sign(v(t)) r v(t)^2) / m dt

	Some fixed friction goes on top to bring the speaker really down to
	zero.
*/

/* Sampling rate as base for all time. */
long rate;
/* Time is counted in sampling intervals of this size.
   (inverse of sampling rate). */
double time_interval;
/* Strength of the last event, compared to full scale.
   This is for capturing an event detected in the
   recovery period. */
double event_strength;
/* Age of last event in intervals. Based on this, the discharge
   force is applied or not, strength of a new discharge computed
   (if event_age > dead time). */
/* Scaling of discharge force, in relation to speaker movement. */
double force_scale;
/* Intervals since last event, if it is recent.  */
long event_age;
/* Number of intervals for that a new event is not detected.
   This is also the duration of the discharge. */
double dead_s;
long dead_time;
/* Number of intervals till recovery after dead time. */
long recover_time;

/* Oscillator properties. */
double mass;     /* mass (inertia) of speaker in kg  */
double spring;   /* spring constant k in N/m         */
double friction; /* friction coefficient in kg/(s m) */
double friction_fixed; /* Speed-independent friction. */

/* State variables. */
double pos;   /* position of speaker [-1:1]       */
double speed; /* speed of speaker movement        */

/* Soft clipping as speaker property. It can only move so far.
   Of course this could be produced by a nicely nonlinear force, too. */
float soft_clip(double x)
{
	const float w = 0.1;
	if(x > 1.-w)
		x =  1. - w*w/(2*w-1.+x);
	else if(x < -1.+w)
		x = -1. + w*w/(2*w-1.-x);
	return x;
}

double sign(double val)
{
	return val < 0 ? -1. : +1;
}

/* Advance speaker position by one time interval, applying the given force. */
float speaker(double force)
{
	double dx, dv;
	/* Some maximal numeric time step to work nicely for small sampling
	   rates. Unstable numerics make a sound, too. */
	double euler_step = 1e-5;
	long steps = 0;
	do
	{
		double step = time_interval-steps*euler_step;
		if(step > euler_step)
			step = euler_step;
		/* dx = v dt
		   dv = (G(t) - k x(t) - sign(v(t)) r v(t)^2) / m dt */
		dx = speed*step;
		dv = (force - spring*pos - sign(speed)*friction*speed*speed)
		   / mass * step;
		pos   += dx;
		speed += dv;
		/* Apply some fixed friction to get down to zero. */
		if(speed)
		{
			double ff = -sign(speed)*friction_fixed/mass*step;
			if(sign(speed+ff) == sign(speed))
				speed += ff;
			else
				speed = 0.;
		}
	} while(++steps*euler_step < time_interval);
	return soft_clip(pos);
}

/* The logic of the Geiger-Mueller counter.
   Given an event (or absence of) for this time interval, return
   the discharge force to apply to the speaker. */
double discharge_force(int event)
{
	double strength = 0.;
	if(event_age >= 0) /* If there was a recent event. */
	{
		/* Apply current event force until dead time is reached. */
		if(++event_age > dead_time)
		{
			long newtime = event_age - dead_time;
			if(newtime < recover_time)
			{
				event_strength = 1.*newtime/recover_time;
			}
			else /* Possible event after full recovery. */
			{
				event_age = -1;
				event_strength = 1.;
			}
		}
		else /* Still serving the old event. */
		{
			strength = event_strength;
			event = 0;
		}
	}
	if(event)
	{
		event_age = 0;
		strength = event_strength;
	}
	return strength*force_scale;
}

int main(int argc, char** argv)
{
	double duration;
	unsigned long samplecount;
	unsigned long events;
	double activity;
	double event_likelihood;
	int thres;
	int outmode = 0;
	if(argc < 2)
	{
		fprintf(stderr
		,	"usage: "
			"%s <bin|txt> [duration_in_seconds] [activity] [sampling_rate]\n"
		, argv[0]);
		fprintf(stderr
		,	"\nDuration can be negative for endless runtime, default activity is 10/s and"
			"\ndefault sampling rate is 48000 Hz\n");
		return 1;
	}
	if(!strcasecmp(argv[1], "bin"))
		outmode = 1;
	else if(!strcasecmp(argv[1], "txt"))
		outmode = 2;
	if(!outmode)
	{
		fprintf(stderr, "%s: invalid output mode given\n", argv[0]);
		return 2;
	}
	duration = argc >= 3 ? atof(argv[2]) : -1;
	/* Activity is in average events per second. */
	activity = argc >= 4 ? atof(argv[3]) : 10;
	if(activity < 0)
		activity = 0.;

	rate = argc >= 5 ? atol(argv[4]) : 48000;
	fprintf(stderr, "rate: %lu\n", rate);
	time_interval = 1./rate;
	event_strength = 1.;
	event_age = -1;
	/* In the order of 100 us. Chose a large time to produce sensible results down
	   to 8000 Hz sampling rate. */
	dead_s = 0.0002;
	dead_time = (long)(dead_s*rate+0.5);
	recover_time = 2*dead_time;
	fprintf(stderr, "dead: %ld recover: %ld\n", dead_time, recover_time);
	/* Let's artitrarily define maximum speaker displacement
	   as 1 mm and specify values accordingly. */
	pos = 0.;
	speed = 0.;
	/* Mass and spring control the self-oscillation frequency. */
	mass = 0.02;
	spring = 1000000;
	/* Some dynamic friction is necessary to keep the unstable numeric
	   solution in check. A kind of lowpass filter, too, of course. */
	friction = 0.02;
	/* Some hefty fixed friction prevents self-oscillation of speaker
	   (background sine wave). But you want _some_ of it. */
	friction_fixed = 20000;
	/* Experimenting, actually. Some relation to the speaker. */
	force_scale = 50000.*mass*0.001/(4.*dead_s*dead_s);

	samplecount = duration >= 0 ? duration/time_interval+0.5 : 0;
	fprintf(stderr, "runtime: %g (%lu samples)\n", duration, samplecount);
	srand(123456);
	event_likelihood = activity*time_interval;
	if(event_likelihood > 1.)
		event_likelihood = 1.;
	thres = RAND_MAX*(1.-event_likelihood);
	fprintf(stderr, "threshold: %i / %i\n", thres, RAND_MAX);

	events = 0;
	for(unsigned long i=0; duration < 0 || i<samplecount; ++i)
	{
		int event = rand()>thres;
		if(event)
			++events;
		float val = speaker(discharge_force(event));
		switch(outmode)
		{
			case 1:
				write(STDOUT_FILENO, &val, sizeof(val));
			break;
			case 2:
				printf("%g\n", val);
			break;
		}
	}
	fprintf(stderr, "%lu events in %g s\n", events, duration);
	return 0;
}
