#ifndef GLIB_THREADING_UTILS_H
#define GLIB_THREADING_UTILS_H

#include <functional>
#include <future>

#include <glib.h>

template <typename CTX, typename ReturnType, typename ArgType>
class MemberMethodWrapper
{
public:
#if defined(DEBUG)
    MemberMethodWrapper(){printf("%s called\n", __PRETTY_FUNCTION__);};
    ~MemberMethodWrapper(){printf("%s called\n", __PRETTY_FUNCTION__);};
#endif
    MemberMethodWrapper& operator=(const MemberMethodWrapper& other) =delete;
    MemberMethodWrapper(const MemberMethodWrapper& other) =delete;
    MemberMethodWrapper(MemberMethodWrapper&& other)
    {
        func = std::move(other.func);
        arg = std::move(other.arg);
        ctxPtr = other.ctxPtr;
        printf("%s called\n", __PRETTY_FUNCTION__);
    };

    using func_type = ReturnType (CTX::*)(ArgType&&);
    func_type func;
    ArgType arg;
    std::shared_ptr<CTX> ctxPtr;
    MemberMethodWrapper(CTX* ctx, func_type f, ArgType&& argument): ctxPtr(ctx->shared_from_this()), func(f), arg(std::move(argument))
    {
#if defined(DEBUG)
        printf("%s called\n", __PRETTY_FUNCTION__);
#endif
    };
    ReturnType operator () () {
        return (*ctxPtr.*func)(std::move(arg));
    };

    std::function<ReturnType()> toStdFunction() {return std::bind(&MemberMethodWrapper::operator(), this);}
};


// decltype((*_this.*function)(std::move(argument))) stands for ReturnType
template <typename ClassType, typename FuncType, typename ArgType>
auto MakeWrapper(ClassType* _this, FuncType function, ArgType&& argument) -> std::shared_ptr<
    MemberMethodWrapper<ClassType, decltype((*_this.*function)(std::move(argument))), ArgType>> {
    return std::make_shared<
            MemberMethodWrapper<ClassType, decltype((*_this.*function)(std::move(argument))), ArgType>>
            (_this, function, std::move(argument));
};

template<typename CTX, typename ReturnType, typename ArgType>
std::future<ReturnType> postToMainThread(std::shared_ptr<MemberMethodWrapper<CTX, ReturnType, ArgType>> f)
{
    std::packaged_task<ReturnType()> task(f->toStdFunction());
    std::future<ReturnType> future = (std::move(task.get_future()));

    typedef struct {
        std::shared_ptr<MemberMethodWrapper<CTX, ReturnType, ArgType>> ptrProtector;
        std::packaged_task<ReturnType()> task;
    } Helper;

    Helper* data = new Helper;
    data->task = std::move(task);
    data->ptrProtector = f;

    g_timeout_add(0, [](void* ptr)
    {
        Helper* data = (Helper*) ptr;
        data->task();

        delete data;
        return G_SOURCE_REMOVE;
    }, data);
    return std::move(future);
}

inline bool isMainThread()
{
    return g_main_context_is_owner (g_main_context_default());
}

#endif
