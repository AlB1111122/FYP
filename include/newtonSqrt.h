//https://www.geeksforgeeks.org/find-root-of-a-number-using-newtons-method/
#pragma once
#include "/usr/share/etl/etl-20.39.4/include/etl/absolute.h"
double sqrt(double n, float l = .5) //defalut to .5 so integer sqrt is accurate
{
    // Assuming the sqrt of n as n only
    double x = n;
 
    // The closed guess will be stored in the root
    double root;
 
    // To count the number of iterations
    int count = 0;
 
    while (1) {
        count++;
 
        // Calculate more closed x
        root = 0.5 * (x + (n / x));
 
        // Check for closeness
        if (etl::absolute(root - x) < l)
            break;
 
        // Update root
        x = root;
    }
 
    return root;
}