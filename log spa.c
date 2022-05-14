#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MAX_ITERATION 16
#define TEST 1000


#include "rand.c"

typedef struct edge
{
    double q;
    double r;
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
        y[i] = 2*y[i]/var;
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
                e[vnode[i][j]].q = qi[k];
            }
        }
        k++;
    }
}


/*
void count_r(Edge *e, int c, int c1, int cnode[][c1])
{
    int i,j,k, f; /// f: if first L read
    double Ltemp;

    for(i=0;i<c;i++)
    {

        for(j=0;j<c1;j++)
        {
            Ltemp = 1;
            for(k=0;k<c1;k++)
            {
                if(k != j)
                {
                    Ltemp *= tanh(0.5*e[cnode[i][k]].q);
                }
            }
            e[cnode[i][j]].r = 2*atanh(Ltemp);
            //printf("%f", e[cnode[i][j]].r );
        }
    }
}*/

double fi(double x)
{
    return -log(tanh(0.5*x));
}

void count_r(Edge *e, int c, int c1, int cnode[][c1])
{
    int i,j,k, f; /// f: if first L read
    double sign, Ltemp;

    for(i=0;i<c;i++)
    {
        for(j=0;j<c1;j++)
        {
            sign = 1;
            Ltemp = 0;
            for(k=0;k<c1;k++)
            {
                if(k != j)
                {
                    if(e[cnode[i][k]].q < 0)
                    {
                        sign = -sign;
                        Ltemp += fi(-e[cnode[i][k]].q);
                    }
                    else{Ltemp += fi(e[cnode[i][k]].q);}
                }
            }
            e[cnode[i][j]].r = sign*fi(Ltemp);
            //printf("%f", e[cnode[i][j]].r );
        }
    }
}

void count_q(Edge *e, double *qi, int v, int v1, int vnode[][v1])
{
    int i,j,k;
    double sig;

    for(i=0;i<v;i++)
    {
        for(j=0;j<v1;j++)
        {
            sig = 0;
            if(vnode[i][j] != -1)
            {
                for(k=0;k<v1;k++)
                {
                    if(k != j && vnode[i][k] != -1)
                    {
                        sig += e[vnode[i][k]].r;
                    }
                }
                e[vnode[i][j]].q = sig + qi[i];
            }

        }
    }
}

void count_Q(Edge *e, double *qi, char *codeword, int v, int v1, int vnode[][v1])
{
    int j,k;
    double Q[v];
    double sig;

    for(j=0;j<v;j++)
    {
        sig = 0;

        for(k=0;k<v1;k++)
        {
            if(vnode[j][k] != -1)
            {
                sig += e[vnode[j][k]].r;
            }
        }
        Q[j] = sig + qi[j];
        if(Q[j] < 0){codeword[j] = '1';}
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
    int vgroup[v], cpass[c], ccheck[c*c1];
    ///vgroup load the amount of c nodes connected to v nodes in txt
    ///cpass count the time c node connected to v node counted
    for(i=0;i<v;i++){fscanf(f, "%d", &vgroup[i]);}
    for(i=0;i<c;i++){fscanf(f, "%d", &ccheck[0]); cpass[i] = 0;}
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
    for(i=0;i<c*c1;i++){fscanf(f, "%d", &ccheck[i]);}
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
            while(!check(codeword, v, c, c1, ccheck));
            for(j=0;j<v;j++){if(codeword[j]=='1'){err++;}}
        }

        err = err/(TEST*v);
        fprintf(f2,"%f\n",err);
    }
    return 1;
}


