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
        y[i] = 1/(1+exp(2*y[i]/var));
    }
}

void initialize(double *qi, Edge *e, int v, int v1, int vnode[][v1])
{
    int i,j, k=0;
    for(i=0;i<v;i++)
    {
        for(j=0;j<v1;j++)
        {
            if(vnode[i][j] != -1)
            {
                e[vnode[i][j]].q1 = qi[k];
                e[vnode[i][j]].q0 = 1 - qi[k];
            }
        }
        k++;
    }
}

void count_r(Edge *e, int c, int c1, int cnode[][c1])
{
    int i,j,k;
    double pi=1;

    for(i=0;i<c;i++)
    {
        for(j=0;j<c1;j++)
        {
            pi = 1;
            for(k=0;k<c1;k++)
            {
                if(k != j){pi *= (1-2*e[cnode[i][k]].q1);}
            }
            e[cnode[i][j]].r0 = 0.5+0.5*pi;
            e[cnode[i][j]].r1 = 1-(e[cnode[i][j]].r0); ///0.5-0.5*pi;
        }
    }
}

void count_q(Edge *e, double *qi, int v, int v1, int vnode[][v1])
{
    int i,j,k;
    double pi0, pi1;

    for(i=0;i<v;i++)
    {
        for(j=0;j<v1;j++)
        {
            pi0 = 1;
            pi1 = 1;
            if(vnode[i][j] != -1)
            {
                for(k=0;k<v1;k++)
                {
                    if(k != j && vnode[i][k] != -1)
                    {
                        pi1 *= e[vnode[i][k]].r1;
                        pi0 *= e[vnode[i][k]].r0;
                    }
                }
                e[vnode[i][j]].q0 = (1-qi[i]) * pi0;
                e[vnode[i][j]].q1 = qi[i] * pi1;
                e[vnode[i][j]].q0 = e[vnode[i][j]].q0 / (e[vnode[i][j]].q0+e[vnode[i][j]].q1);
                e[vnode[i][j]].q1 = 1 - e[vnode[i][j]].q0;
            }
        }
    }
}

void count_Q(Edge *e, double *qi, char *codeword, int v, int v1, int vnode[][v1])
{
    int j,k;
    double Q0[v], Q1[v];
    double pi0, pi1;

    for(j=0;j<v;j++)
    {
        pi0 = 1;
        pi1 = 1;

        for(k=0;k<v1;k++)
        {
            if(vnode[j][k] != -1)
            {
                pi1 *= e[vnode[j][k]].r1;
                pi0 *= e[vnode[j][k]].r0;
            }
        }
        Q0[j] = (1-qi[j]) * pi0;
        Q1[j] = qi[j] * pi1;
        Q0[j] = Q0[j]/(Q0[j]+Q1[j]);
        Q1[j] = 1-Q0[j];
        if(Q1[j] > Q0[j]){codeword[j] = '1';}
        else{codeword[j] = '0';}
    }
}

int check(char *codeword, int v, int c, int c1, int cnode[])
{return 0;
    int i,j;
    int sum;

    for(i=0;i<c;i++)
    {
        sum = 0;
        for(j=0;j<c1;j++)
        {
            if(codeword[cnode[i*c1+j]%v] == '1'){sum++;} //i-th c node
        }
        if(sum%2 != 0){return 0;}
    }
    return 1;
}

int main()
{
    ///read H
    FILE *f = fopen("H.txt", "r");
    int v, c;   ///v,c nodes amount
    int v1, c1; ///max edges connect to v,c nodes
    fscanf(f, "%d %d %d %d", &v, &c, &v1, &c1);
    int cnode[c*c1]; ///connection group of c nodes
    int vnode[v][v1];///connection group of v nodes
    int i,j;
    int temp;
    int vgroup[v], cpass[c];
    ///vgroup load the amount of c nodes connected to v nodes in txt
    ///cpass count the time c node connected to v node counted
    for(i=0;i<v;i++){fscanf(f, "%d", &vgroup[i]);}
    for(i=0;i<c;i++){fscanf(f, "%d", &temp); cpass[i] = 0;}
    for(i=0;i<c*c1;i++){cnode[i]=i;}  ///set the number of edge
    for(i=0;i<v;i++)  ///save the edges in group
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
    }
    fclose(f);
    ///end read H

    double qi[v]; /// first q(1)
    char codeword[v];
    Edge e[c*c1];

    int t;
    double SNR;
    double err;
    FILE *f2 = fopen("err.txt", "w");
    for(SNR=1.3;SNR<2.3;SNR+=0.1)
    {
        err = 0;
        for(t=0;t<TEST;t++) ///test time
        {
            receive(qi, v, SNR); /// initialize
            initialize(qi, e, v, v1, vnode);


            ///iteration start
            i=0;
            do
            {
                i++;
                if(i > MAX_ITERATION){/*printf("\nNot converge\n");*/break;}
                //printf("iteration %d:\n", i);
                count_r(e, c, c1, cnode);
                count_q(e, qi, v, v1, vnode);
                //printf("Q :\t");
                count_Q(e, qi, codeword, v, v1, vnode);
                //printf("\n");
                //printf("c = \n");
                //for(j=0;j<v;j++){/*printf("%c ", codeword[j]);*/}
            }
            while(!check(codeword, v, c, c1, cnode));
            for(j=0;j<v;j++){if(codeword[j]=='1'){err++;}}
        }

        err = err/(TEST*v);
        fprintf(f2,"%f\n",err);
    }

}
