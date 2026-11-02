#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>

int main(){
    struct timeval start, end;

    long long min_diff = 1000000000LL;
    long long zero_diffs = 0;

    int N = 100000;

    for(int i=0; i<N; i++){
        gettimeofday(&start, NULL);
        gettimeofday(&end, NULL);

        long long diff = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
        
        if(diff == 0){
            zero_diffs++;
        }
        if(diff > 0 && diff < min_diff){
            min_diff = diff;
        } 
    }
    printf("Minimum positive difference: %lld us\n",
           min_diff);

    printf("Zero-difference measurements: %lld / %d\n",
           zero_diffs, N);

    return 0;
}