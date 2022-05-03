#pragma once
#include <math.h>

/**** NUMERIC DATA FUNCTIONS ****/

// find first decimals
// @return positive = decimals, negative = integers
// @note these functions are not currenly used
static int find_first_decimals (double number) {
    if (number == 0.0) return (10); // what to do with this rare case? round to 10 decimals
    else return(-floor(log10(fabs(number)))); // could do this faster than with the log with a while if this is a problem
}

// find deicmal for the specific number and significant digits
// @param signif = number of significant digits, 1 by default
// @param decimals_only = always round integers to 1
// @param limit = highest number of decimals that should be used
static int find_signif_decimals (double number, uint signif = 1, bool decimals_only = false, int limit = 10) {
    int decimals = find_first_decimals (number) + signif - 1;
    if (decimals_only && decimals < 0) decimals = 0;
    if (decimals > limit) decimals = limit;
    return(decimals);
}

// round a number to the specified decimals
static double round_to_decimals (double number, int decimals) {
    double factor = pow(10.0, decimals);
    return(round(number * factor) / factor);
}

// print a number to the specified decimals
static void print_to_decimals (char* target, int size, double number, int decimals) {

    // round
    double rounded_number = round_to_decimals(number, decimals);

    // print
    if (decimals < 0) decimals = 0;
    char number_pattern[6];
    snprintf(number_pattern, sizeof(number_pattern), "%%.%df", decimals);
    snprintf(target, size, number_pattern, rounded_number);
}

// print a number to the specific significan digits
static void print_to_signif (char* target, int size, double number, int signif) {
    print_to_decimals(target, size, number, find_signif_decimals(number, signif));
}

/**** Value statistics ****/

// implemented based on Welford's algorithm
// forward declaration for component
struct RunningStats;
struct RunningStats {

    int n;
    double mean;
    double M2;

    public:

        RunningStats() {
          clear();
        }

        void clear () {
            n = 0;
            mean = 0.0;
            M2 = 0.0;
        }

        void set(RunningStats rs) {
            n = rs.n;
            mean = rs.mean;
            M2 = rs.M2;
        }

        void add(double x) {
            n++;
            double delta = x - mean;
            mean += delta / n;
            M2 += delta * (x - mean);
        }

        int getN() {
            return n;
        }

        double getMean() {
            return mean;
        }

        double getVariance() {
          // technically not defined for n = 1, returning 0.0 instead
          return ( (n > 1) ? M2 / (n - 1) : 0.0 );
        }

        double getStdDev() {
            return sqrt( getVariance() );
        }


};
