#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <time.h>

// global state variables
int	    NowYear;		// 2021 - 2026
int	    NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain this month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches this month
int	    NowNumDeer;		// number of deer in the current population this month
int     NowNumWolves;     // number of wolf in the current population this month

// global constants
const float GRAIN_GROWS_PER_MONTH =		9.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;

// code for barrier's that work in g++ and visual studio
omp_lock_t	Lock;
int		NumInThreadTeam;
int		NumAtBarrier;
int		NumGone;

unsigned int seed;  // random seed

float Ranf( unsigned int *seedp,  float low, float high )
{
        float r = (float) rand_r( seedp );              // 0 - RAND_MAX

        return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}


int Ranf( unsigned int *seedp, int ilow, int ihigh )
{
        float low = (float)ilow;
        float high = (float)ihigh + 0.9999f;

        return (int)(  Ranf(seedp, low,high) );
}

// a different random number sequence every time you run it:
unsigned int TimeOfDaySeed( )
{
	struct tm y2k = { 0 };
	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time_t  timer;
	time( &timer );
	double seconds = difftime( timer, mktime(&y2k) );
	unsigned int seed = (unsigned int)( 1000.*seconds );    // milliseconds
	return seed;
}

float SQR( float x )
{
    return x*x;
}

void Deer( ) 
{
    while( NowYear < 2027 )
    {
        int nextDeerNum = NowNumDeer;
        if (nextDeerNum > NowHeight) {
            nextDeerNum--;
        } else if (NowNumDeer < NowHeight) {
            nextDeerNum++;
        }

        if (NowNumWolves > (.20*NowNumDeer)) {
            nextDeerNum -= 2;
        } else if (NowNumWolves <= (.20*NowNumDeer)) {
            nextDeerNum++;
        }

        if (nextDeerNum < 0)
            nextDeerNum = 0;

        #pragma omp barrier

        NowNumDeer = nextDeerNum;

        #pragma omp barrier
        #pragma omp barrier
    }
}

void Grain( ) 
{
    while( NowYear < 2027 )
    {
        // calculate growth factor
        float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
        float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * (ONE_DEER_EATS_PER_MONTH);

        // height can't be negative
        if( nextHeight < 0. )
            nextHeight = 0.;

        #pragma omp barrier

        // save height to global variable
        NowHeight = nextHeight;

        #pragma omp barrier
        #pragma omp barrier
    }
}

void Wolves( )
{
    while( NowYear < 2027 )
    {
        int nextWolfNum = NowNumWolves;
        if (nextWolfNum == 0 && NowNumDeer >= 5) {
            nextWolfNum = 1;
        } else if (nextWolfNum > (.20*NowNumDeer)) {
            // printf("Lose a wolf\n");
            nextWolfNum -= 2;
        } else if (nextWolfNum < (.20*NowNumDeer)) {
            nextWolfNum++;
        }

        // wolves can't be negative
        if( nextWolfNum < 0 )
            nextWolfNum = 0;

        #pragma omp barrier

        NowNumWolves = nextWolfNum;

        #pragma omp barrier
        #pragma omp barrier
    }
}

void Watcher( )
{
    while( NowYear < 2027 )
    {
        #pragma omp barrier
        #pragma omp barrier
        
        float metricHeight = NowHeight * 2.54;
        float metricPrecip = NowPrecip * 2.54;
        float metricTemp = (5./9.)*(NowTemp-32);
        printf("%d/%d; %.2f; %.2f; %d; %.2f; %d \n", NowMonth+1, NowYear, metricTemp, metricPrecip, NowNumDeer, metricHeight, NowNumWolves);

        // increment month
        NowMonth++;

        // increment caladar year
        if (NowMonth == 12) {
            NowMonth = 0;
            NowYear++;
        }

        // set the temperature for the month   
        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );
        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

        // set the precipiation for the month
        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
        
        // precipitation cant be negative
        if( NowPrecip < 0. )
	        NowPrecip = 0.;

        #pragma omp barrier
    }
}

int main()
{
    // check if openmp is supported
    #ifndef _OPENMP
        fprintf( stderr, "OpenMP is not supported here -- sorry.\n" );
        return 1;
    #endif

    seed = TimeOfDaySeed();

    // starting date and time
    NowMonth = 0;
    NowYear = 2021;

    // starting state
    NowNumDeer = 10;
    NowHeight = 20.;
    NowNumWolves = 0;
    NowPrecip = 0;		// inches of rain this month
    NowTemp = -5;

    
    printf("%d/%d; %.2f; %.2f; %d; %.2f; %d \n", NowMonth+1, NowYear, metricTemp, metricPrecip, NowNumDeer, metricHeight, NowNumWolves);

    omp_set_num_threads( 4 );	// same as # of sections
    #pragma omp parallel sections
    {
        
        #pragma omp section
        {
            Deer( );
        }

        #pragma omp section
        {
        	Grain( );
        }

        #pragma omp section
        {
        	Wolves( );
        }

        #pragma omp section
        {
            // print the results in a way that can be brought right into excell
        	Watcher( );
        }


    }       // implied barrier -- all functions must return in order
        // to allow any of them to get past here
    return 0;
}
