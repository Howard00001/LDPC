#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MAX_ITERATION 1
#define TEST 1000

#include "rand.c"

typedef struct edge
{
    double q0, q1;
    double r0, r1;
}Edge;

void receive(double *y, int v, double SNR) /// initialize y
{
    int i;

    double n[v];
	double sigma = pow(10,(-SNR)/20); ///standard deviation
    double var = pow(sigma, 2);

	///initialize the rand seed
	rand_init();


	for (i = 0; i < v; ++i) {
	    n[i] = gaussian() * sigma;       /// generate v samples of gaussian noise with variance sigma^2.
	}

    for(i=0; i<v; i++)
    {
        y[i]= 1+n[i];                      ///receive 1 + noise

    }
}


int main()
{
    ///read H
    FILE *f = fopen("H.txt", "r");
    int v, c;   ///v,c nodes amount
    int v1, c1; ///max edges connect to v,c nodes
    fscanf(f, "%d %d %d %d", &v, &c, &v1, &c1);
/*    int cnode[c*c1]; ///connection group of c nodes
    int vnode[v][v1];///connection group of v nodes
    int i,j;
    int temp;
    int vgroup[v], cpass[c];
    ///vgroup load the amount of c nodes connected to v nodes in txt
    ///cpass count the time c node connected to v node counted
    for(i=0;i<v;i++){fscanf(f, "%d", &vgroup[i]);}
    for(i=0;i<c;i++){fscanf(f, "%d", &temp); cpass[i] = 0;}
    for(i=0;i<c*c1;i++){cnode[i]=i;}  ///set the number of edge
    /*for(i=0;i<v;i++)  ///save the edges in group
    {
        for(j=0;j<v1;j++)
        {
            if(j>=vgroup[i]){vnode[i][j] = -1;}
            else
            {
                fscanf(f, "%d", &temp);
                vnode[i][j] = (temp-1)*c1 + cpass[temp-1];
                cpass[temp-1] += 1;
            }
        }
    }*/
    fclose(f);
    ///end read H

    double qi[v]; /// first q(1)
    //char codeword[v];
    //Edge e[c*c1];

    int t,j;
    double SNR;
    double err;
    FILE *f2 = fopen("err.txt", "w");
    for(SNR=1.3;SNR<2.3;SNR+=0.1)
    {
        err = 0;
        for(t=0;t<TEST;t++) ///test time
        {
            receive(qi, v, SNR); /// initialize
            //initialize(qi, e, v, v1, vnode);

            //count_Q(e, qi, codeword, v, v1, vnode);

            for(j=0;j<v;j++){if(qi[j]<0){err++;}}
        }

        err = err/(TEST*v);
        fprintf(f2,"%f\n",err);
    }

}
