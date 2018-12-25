
int myErrorOperate(char const * const error_str,int error_line,int error_exit=1)
{
    #include<stdio.h>
    printf("%d\n",error_line);
    if(error_exit==1)
        exit(1);
    return 0;
}
#ifndef PTHREADNUM
    #define PTHREADNUM 5
#endif
namespace mpnsp
{
    struct  memNode
    {
        char *memory;
        char *head;
        bool deleted;
        uint size;
        uint used;
        uint *ptrArr;
        uint top;
        uint nptr;//下一个node的节点号
        uint fptr;//上一个node的节点号
    };
    class memPool;
    std::map<int,memPool*>memPoolMap[PTHREADNUM];

    class memPool{
    private:
        int unitSize;//单位大小
        typedef unsigned int uint;
        long topPtr;//指向一个空闲内存的块号
        ///31~21共11位表示node号，20~0共21位表示块内编号，块内最多盛放2M个。

        #define OffsetBits 21
        #define MaskOffsetNumb 07777777  //块内编号掩码为：07777777

        #define InitNumInNode 2
        #define MaxOffset (1024*1024)
        #define MaxNodeNum 127    //127  这个值的最大值跟node编号位数有关(2*1024-1)，但是大了影响效率
        #define MaxAllocMem 128*1024*1024  //(128*1024*1024) //单位B     //内存node内存上限（实际要大一个单位用来内存对齐浪费）
        #define EnableAutoClean 1   //允许自动删除小节点以提高速度 ，注意是自动。。。。如果此定义为0，用户仍然可以手动调用clean来完成这个功能
        #define AutoCleanSize (maxNumInNode>>4)    //自动清理的上限,默认16倍 (inclusive)
        uint nodeNum;//当前nodeNum
        uint deletedNum;
        uint MaxAllocNum;
        uint maxNumInNode;
        memNode **nodeArr=NULL;
        bool status;    //pool is available?
        void freeMemNode(int idx); //删除被标记为deleted的node

        int createNewNode(uint num);//返回值是新开辟的位置

    public:
        ~memPool();
        void memPoolInit();
        memPool(int nodeSize,int initNum=InitNumInNode);
        void* allocMemFromPool();
        void freeMemToPool(void *p);
        int clean(long long size=-1);
        void debug(); //debug函数在cpp文件中
    };
    int do_createMemPool(int nodeSize,int initNum,std::map<int,memPool*>&memPoolMap); //初始个数
    template<typename T>inline int createMemPool(int tid,int initNum=InitNumInNode);
}
template<typename T>    void freeNode(T* p,int tid);
template<typename T>    T* allocNode(int tid);
#include"mempool.cpp"



