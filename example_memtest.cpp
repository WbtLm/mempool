#include<bits/stdc++.h>
using namespace std;

#define PTHREADNUM 5  //要在mempool.h前面定义PTHREADNUM表示有多少个线程。
#include"mempool.h"
using namespace mpnsp;
typedef struct node{
    char buf[64];
}node;

int main()
{
    node *p[100000];
    int n=1e4;
    int tid=0;//表示第几个线程，而不是单纯的tid。mempool中的map数组会创建PTHREADNUM个map。
    for(int i=0;i<n;i++)
        p[i]=allocNode<node>(tid);
    memPoolMap[tid][sizeof(node)]->debug();
    for(int i=0;i<n;i++)
        freeNode(p[i],tid);
    memPoolMap[tid][sizeof(node)]->debug();
    for(int i=0;i<n;i++)
        p[i]=allocNode<node>(tid);
    memPoolMap[tid][sizeof(node)]->debug();
    for(int i=0;i<n;i++)
        freeNode(p[i],tid);
    memPoolMap[tid][sizeof(node)]->debug();
    cout<<"end"<<endl;
}
