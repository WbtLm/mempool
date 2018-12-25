int mpnsp::memPool::clean(long long size/*=-1*/)
{
    // cout<<"--------------AutoCleanSize="<<AutoCleanSize<<endl;
    if(size<0)
    {
        size=AutoCleanSize;
    }
    if(size>=maxNumInNode)
    {
        return -1;  //把最大的都删了就不合适了
    }
    int cnt=0;
    for(int i=1;i<=MaxNodeNum;i++)
    {
        if(nodeArr[i] && nodeArr[i]->size<=AutoCleanSize && !nodeArr[i]->deleted)
        {
            nodeArr[i]->deleted=1;
            cnt++;
        }
    }
    nodeNum-=cnt;
    deletedNum+=cnt;
    return cnt;
}

void mpnsp::memPool::freeMemNode(int idx) //删除被标记为deleted的node
{
    if(__builtin_expect(MaxNodeNum<idx,0))
    {
        myErrorOperate("fatal error ! freeMemNode.nodeNumidx.",__LINE__,1);
        return;
    }
    if(__builtin_expect(!nodeArr[idx],0))
    {
        myErrorOperate("freeMemNode.node is already NULL.",__LINE__,0);
        return;
    }
    if(!__builtin_expect(nodeArr[idx]->deleted,1))
    {
        myErrorOperate("freeMemNode.deleted must be true.",__LINE__,0);
        return;
    }
    int fptr=nodeArr[idx]->fptr;
    int nptr=nodeArr[idx]->nptr;
    nodeArr[fptr]->nptr=nptr;
    if(nptr<=MaxNodeNum)
    {
        nodeArr[nptr]->fptr=fptr;
    }
    free(nodeArr[idx]->memory);
    free(nodeArr[idx]->ptrArr);
    free(nodeArr[idx]);
    nodeArr[idx]=NULL;
    deletedNum--;
}
int mpnsp::memPool::createNewNode(uint num)//返回值是新开辟的位置
{
    if(__builtin_expect(nodeNum+deletedNum>=MaxNodeNum,0))
    {
        myErrorOperate("memPool.nodes are too many.",__LINE__,0);
        return -1;
    }
    if(num<=1)
    {
        myErrorOperate("createNewNode.num must >1",__LINE__,1);
    }
    int i;
    for(i=0;i<MaxNodeNum && nodeArr[i]!=NULL;i++);//ok 寻找一个空节点
    memNode *node=(memNode*)malloc(sizeof(memNode));
    if(__builtin_expect(node==NULL,0))
        return -1;
    nodeArr[i]=node;
    node->used=0;
    node->deleted=0;
    node->memory=(char*)malloc(unitSize*(num+1));
    if(__builtin_expect(node->memory==NULL,0))
        return -1;
    MaxAllocNum=std::max(num,MaxAllocNum);
    node->head=node->memory+unitSize-((long long)node->memory%unitSize);//内存对齐
    node->size=num;
    node->ptrArr=(uint*)malloc(num*sizeof(uint*));
    if(__builtin_expect(node->ptrArr==NULL,0))
        return -1;
    node->fptr=i-1;
    node->nptr=nodeArr[i-1]->nptr;
    nodeArr[i-1]->nptr=i;
    if(node->nptr < MaxNodeNum)
    {
        nodeArr[node->nptr]->fptr=i;
    }

    for(int j=1;j<=num;j++)//ok
    {
        node->ptrArr[j-1]=j;
    }
    node->top=0;
    nodeNum++;
    return i;
}
mpnsp::memPool::~memPool()
{
    for(int i=0;i<MaxNodeNum;i++)
    {
        if(nodeArr[i])
        {
            free(nodeArr[i]->memory);
            free(nodeArr[i]->ptrArr);
            free(nodeArr[i]);
        }
    }
    free(nodeArr);
}
void mpnsp::memPool::memPoolInit()
{
    int s=sizeof(memNode*)*(MaxNodeNum+1);
    nodeArr=(memNode**)malloc(s);
    if(__builtin_expect(nodeArr==NULL,0))
    {
        myErrorOperate("memPool.nodeArr.create fail",__LINE__,0);
        return;
    }
    memset(nodeArr,0,s);
    //初始化头结点
    memNode *node=(memNode*)malloc(sizeof(memNode));
    if(__builtin_expect(node==NULL,0))
    {
        myErrorOperate("memPool.create head node fail",0);
        return;
    }
    nodeArr[0]=node;
    node->used=0;
    node->deleted=0;
    node->memory=NULL;
    node->head=NULL;
    node->size=0;
    node->ptrArr=NULL;
    node->fptr=0;
    node->nptr=MaxNodeNum+1;
    node->top=-1;
        //创建第一个节点

    int ret=createNewNode(maxNumInNode);
    if(ret<0)
    {
        myErrorOperate("memPool.createNewNode.fail",__LINE__,1);
    }
    topPtr=ret<<OffsetBits;//指向新开辟的node
    status=1;
}
mpnsp::memPool::memPool(int nodeSize,int initNum/*=InitNumInNode*/)
{
    status=0;
    unitSize=nodeSize;
    topPtr=-1;
    nodeNum=0;
    deletedNum=0;
    maxNumInNode=initNum;
    MaxAllocNum=MaxAllocMem/nodeSize;
    if(MaxAllocMem<16)
    {
        myErrorOperate("memPool.MaxAllocMem is too small.",__LINE__,1); //开发阶段就很容易找到的错误
        return;
    }
    if(maxNumInNode>MaxAllocNum)
    {
        maxNumInNode=MaxAllocNum;
    }
    memPoolInit();
}
void* mpnsp::memPool::allocMemFromPool()
{
    if(__builtin_expect(status==0,0))
    {
        memPoolInit();
        if(status==0)
        {
            return malloc(unitSize);
        }
    }
    if(topPtr<=-1)
    {
        maxNumInNode=std::min(maxNumInNode<<1,MaxAllocNum);
        int ret=createNewNode(maxNumInNode);
        topPtr=ret<<OffsetBits;
        if(ret==-1)
        {
            topPtr=-1;
            myErrorOperate("allocMemFromPool.there is too many unit. So . Have to call to malloc.",__LINE__,0);
            return malloc(unitSize);
        }
    }
    #define alloc_head(idx) (node->head+idx*unitSize)
    #define alloc_ptr(idx) (node->ptrArr+idx)
    uint nodeIdx=topPtr>>OffsetBits;
    uint offsetIdx=topPtr & MaskOffsetNumb;
    if(nodeArr[nodeIdx]==NULL || nodeArr[nodeIdx]->deleted)      //如果当前节点不存在 or 被删除了
    {
        for(nodeIdx=0;nodeIdx<=MaxNodeNum;)
        {
            if(__builtin_expect(nodeArr[nodeIdx]==NULL,0))
                nodeIdx++;
            else if(nodeArr[nodeIdx]->deleted==1 || nodeIdx==0)
            {
                nodeIdx=nodeArr[nodeIdx]->nptr;
            }
            else
                break;
        }
        if(nodeIdx>MaxNodeNum)
        {
            topPtr=-1;
            return allocMemFromPool();
        }
        offsetIdx=nodeArr[nodeIdx]->top;
    }
    memNode *node=nodeArr[nodeIdx];

    void*retPtr=alloc_head(offsetIdx);
    uint &nodeTop=node->top;
    if(__builtin_expect(node->used >= node->size,0)) //当前节点容纳不下
    {
        int i;
        for(i=nodeArr[0]->nptr;i<=MaxNodeNum;)
        {
            if(__builtin_expect(nodeArr[i]==NULL,0))
                i++;
            else if(nodeArr[i]->deleted==1 || nodeArr[i]->used >= nodeArr[i]->size)
            {
                i=nodeArr[i]->nptr;
            }
            else //能容得下
            {
                node=nodeArr[i];
                retPtr=node->head+node->top;
                node->used++;

                node->top=*alloc_ptr(node->top);
                topPtr=i<<OffsetBits;
                topPtr|=node->top;
                return retPtr;
            }
        }
        //当前容纳不下且也没有节点能容纳下
        topPtr=-1;
        return allocMemFromPool();  //一层递归
    }
    else
    {
        node->used++;
        node->top=*alloc_ptr(offsetIdx);
        topPtr&=~MaskOffsetNumb;
        topPtr|=node->top;
    }
    return retPtr;
}
void mpnsp::memPool::freeMemToPool(void *p)
{
    #define free_end(idx) (nodeArr[idx]->head+unitSize*nodeArr[idx]->size)
    #define free_head(idx) (nodeArr[idx]->head)
    int i;
    char *ptr=(char*)p;
    for(i=nodeArr[0]->nptr;i<=MaxNodeNum;)
    {
        if(__builtin_expect(nodeArr[i]==NULL,0))
            i++;
        else if(!(p>=free_head(i) && p<free_end(i)))
        {
            i=nodeArr[i]->nptr;
        }
        else
        {
            memNode *node=nodeArr[i];
            uint idx=(ptr-node->head)/unitSize;
            *alloc_ptr(idx)=node->top;
            node->top=idx;
            topPtr=i<<OffsetBits;
            topPtr|=node->top;
            node->used--;
            #if EnableAutoClean
            if(__builtin_expect(nodeArr[i]->deleted == 0 && nodeArr[i]->size <= AutoCleanSize,0))
            {
                nodeArr[i]->deleted=1;
                nodeNum--;
                deletedNum++;
            }
            #endif
            if(!nodeArr[i]->used && nodeArr[i]->deleted)
            {
                //如果打了deleted标记的node已经没有成员了，就删除
                freeMemNode(i);
            }
            return;
        }
    }
    myErrorOperate("freeMemToPool.mem.Release mem to OS instand of to pool.",__LINE__,0);
    free(p);
}

template<typename T>
T* allocNode(int tid)
{
    #ifndef MEMPOOLOFF
    using namespace mpnsp;
    if(__builtin_expect(!memPoolMap[tid].count(sizeof(T)),0))
    {
        createMemPool<T>(tid);
    }
    memPool &pool=*memPoolMap[tid][sizeof(T)];
    return (T*)pool.allocMemFromPool();
    #else
    return malloc(sizeof(T));
    #endif
}

template<typename T>
void freeNode(T* p,int tid)
{
    using namespace mpnsp;
    if(memPoolMap[tid].count(sizeof(T)))
    {
        memPool *pool=memPoolMap[tid][sizeof(T)];
        pool->freeMemToPool(p);
        if(memPoolMap[tid].count(sizeof(T))==0)
        {
            myErrorOperate("free.err",__LINE__);
        }
    }
    else
        free(p);
    return;
}

int mpnsp::do_createMemPool(int nodeSize,int initNum,std::map<int,memPool*>&memPoolMap)//初始个数
{
 //   cout<<"do_create.初始化内存池:"<<"size="<<nodeSize<<endl;
    if(memPoolMap.count(nodeSize))//不允许重复建立
    {
        myErrorOperate("不允许重复建立",__LINE__,1);
        return 1;
    }
    memPool *pool=new memPool(nodeSize,initNum);
    memPoolMap[nodeSize]=pool;
}

template<typename T>
inline int mpnsp::createMemPool(int tid,int initNum/*=InitNumInNode*/)
{
    return do_createMemPool(sizeof(T),initNum,memPoolMap[tid]);
}
void mpnsp::memPool::debug()
    {
        #include<iostream>
        using namespace std;
        cout<<"maxNumInNode="<<maxNumInNode<<endl;
        cout<<"AutoCleanSize="<<AutoCleanSize<<endl;
        cout<<"topPrt=";
        printf("%x\n",topPtr);
        for(int i=0;i<=MaxNodeNum;i++)
        {
            cout<<"["<<i<<"]"<<"deleted=";
            if(nodeArr[i]==NULL)
            {
                cout<<"NULL"<<endl;
            }
            else
            {
                cout<<nodeArr[i]->deleted<<" ";
                printf("%p\t",nodeArr[i]->head);
                cout<<"fptr="<<nodeArr[i]->fptr<<" nptr="<<nodeArr[i]->nptr;
                cout<<" used/size="<<nodeArr[i]->used<<"/"<<nodeArr[i]->size<<endl;
            }
        }
        return;
    }
