#include <glib_threading_utils.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>

class B
{
public:
    std::mutex m;
    bool empty{0};
    static int instanceCount;
    B(){std::lock_guard<std::mutex> guard(m); printf("%s called, total instance count: %d\n", __PRETTY_FUNCTION__, ++instanceCount);};
    ~B(){std::lock_guard<std::mutex> guard(m); printf("%s called, total instance count: %d\n", __PRETTY_FUNCTION__, empty ? instanceCount : --instanceCount);};
    B& operator=(const B&){printf("COPY!! %s called\n", __PRETTY_FUNCTION__);};
    B(const B&){std::lock_guard<std::mutex> guard(m); printf("COPY constructor %s called, total instance count: %d\n", __PRETTY_FUNCTION__, ++instanceCount);};
    B(B&& other){other.empty = true; printf("MOVE %s called\n", __PRETTY_FUNCTION__);};
};
int B::instanceCount = 0;

class A : public std::enable_shared_from_this<A>
{
public:
    int ExecuteTestFuncOnMainThread(B&& );

    std::mutex m;
    bool empty{0};
    static int instanceCount;
    A(){std::lock_guard<std::mutex> guard(m); printf("%s called, total instance count: %d\n", __PRETTY_FUNCTION__, ++instanceCount);};
    ~A(){std::lock_guard<std::mutex> guard(m); printf("%s called, total instance count: %d\n", __PRETTY_FUNCTION__, empty ? instanceCount : --instanceCount);};
    A& operator=(const A&){printf("COPY!! %s called\n", __PRETTY_FUNCTION__);};
    A(const A&){std::lock_guard<std::mutex> guard(m); printf("COPY constructor %s called, total instance count: %d\n", __PRETTY_FUNCTION__, ++instanceCount);};
    A(A&& other){other.empty = true; printf("MOVE %s called\n", __PRETTY_FUNCTION__);};
};
int A::instanceCount = 0;

int A::executeTestFuncOnMainThread(B&& b)
{
    printf("isMainThread() : %s %p\n", isMainThread()? "true":"false", this);
    if (!isMainThread())
    {
        auto t = MakeWrapper(this, &A::executeTestFuncOnMainThread, std::move(b));
        auto future = postToMainThread(t);
        //return future.get();
        return 42;
    }
    assert(isMainThread());
    sleep(1);
    printf("isMainThread() : %s %p\n", isMainThread()? "true":"false", this);
    return 42 + 42;
}


void runTestOnThread()
{
    assert(!isMainThread());
    auto a = std::make_shared<A>();
    int r = a->executeTestFuncOnMainThread(B());
    printf("returned value: %d\n", r);
}

int runTestMainThread(void*)
{
    assert(isMainThread());
    std::thread t(runTestOnThread);
    t.detach();
    return G_SOURCE_REMOVE;
}

int main()
{
    setbuf(stdout,0);
    GMainLoop* ml = g_main_loop_new(NULL, FALSE);
    g_timeout_add(1000, runTestMainThread, nullptr);

    g_main_loop_run(ml);
}
