#pragma once
#include <type_traits>
namespace MStd {
    //c++函数类function类的类似封装，Ty为返回值类型，可变参数模板为接受的参数
    template<class Ty, class... Args>
    class Function {
        /*核心思想，定义一个能够接受任何参数（主要是用于接受lambda表达式函数和函数指针，普通参数
        * 调用invoke函数时会报错）的内部类
        */
        //定义函数接口，定义一个invoke函数和clone函数
        class IFunction {
        public:
            virtual Ty invoke(Args&&... args) const = 0;
            virtual IFunction* clone() const = 0;
            virtual ~IFunction() {}
        };
        //定义IFinction的实现类模板，模板参数T用于接受函数指针和lambda表达式
        template<class T>
        class function : public IFunction
        {
            T func;
        public:
            //构造函数
            function(T func) :func(func) {}
            //实现（重写）invoke函数，直接让参数使用调用运算符，可变参数使用std::forward进行传参
            Ty invoke(Args&&... args) const override { 
                return func(std::forward<Args>(args)...);
            }
            //实现（重写）clone函数，返回一个新的IFunction接口指针
            IFunction* clone()const override {
                return new function(this->func);
            }
        };
        //使用IFunction接口指针解决不能直接定义实现类function类模板对象的问题
        IFunction* f;
    public:
        //为IFuncion接口取一个别名
        using InnerFunc = IFunction;
        //无参构造函数
        Function() :f(nullptr) {}
        //定义参数模板，为IFunction的实现类赋值并用于私有成员f接受
        template<class T>
        Function(T f) : f(new function<T>(f)) {}
        //构造函数中使用了new进行构造，所以重载拷贝构造函数进行深拷贝
        Function(const Function& function) {
            this->f = function.f->clone();
        }
        //invoke函数用于调用成员f的invoke函数-调用被封装函数
        Ty invoke(Args... args)const {
            return f->invoke(std::forward<Args>(args)...);
        }
        //返回封装函数的成员对象
        InnerFunc* func() const{
            return f;
        }
        //重新注册一个函数
        template<class T>
        void assign(T f) {
            if (this->f) {
                delete this->f;
            }
            this->f = new function<T>(f);
        }
        //重载析构函数释放new出来的内存
        ~Function() {
            if (f)
                delete f;
        }
        //重载()调用运算符，使的此类的对象能够像函数那样使用，内部调用此类的invoke函数间接调用成员f的invoke函数
        Ty operator ()(Args... args)const {
            return this->invoke(std::forward<Args>(args)...);
        }
        //使用本对象的f成员克隆一个新的Function对象
        Function clone()const {
            Function f;
            f.f = this->f->clone();
            return f;
        }
        //由于使用了new开辟了内存，需要重载=运算符
        Function& operator =(Function&& other) {
            if (this->f)delete this->f;
            this->f = other.f->clone();
            return *this;
        }
        Function& operator =(const Function& other) {
            if (this->f)delete this->f;
            this->f = other.f->clone();
            return  *this;
        }
        //两个函数对象彼此交换各自的f成员
        void swap(Function& function) {
            Function t = *this;
            this->assign(function);
            function.assign(t);
        }
    };
    template<class R>
    class CppFunction;
    template<typename R, typename... Args>
    class CppFunction<R(Args...)> {
    public:
        CppFunction() : ptr(nullptr), deleter(nullptr) {}

        template<typename F>
        CppFunction(F&& f) : ptr(new impl<F>(std::forward<F>(f))), deleter(&CppFunction::delete_impl<F>) {}

        CppFunction(const CppFunction& other) : ptr(other.ptr ? other.ptr->clone() : nullptr), deleter(other.deleter) {}

        CppFunction(CppFunction&& other) noexcept : ptr(other.ptr), deleter(other.deleter) {
            other.ptr = nullptr;
            other.deleter = nullptr;
        }

        CppFunction& operator=(const CppFunction& other) {
            CppFunction(other).swap(*this);
            return *this;
        }

        CppFunction& operator=(CppFunction&& other) noexcept {
            CppFunction(std::move(other)).swap(*this);
            return *this;
        }

        ~CppFunction() {
            if (ptr) deleter(ptr);
        }

        R operator()(Args... args) const {
            return ptr->invoke(std::forward<Args>(args)...);
        }

        explicit operator bool() const noexcept {
            return ptr != nullptr;
        }

        void swap(CppFunction& other) noexcept {
            std::swap(ptr, other.ptr);
            std::swap(deleter, other.deleter);
        }

    private:
        class impl_base {
        public:
            virtual ~impl_base() {}
            virtual R invoke(Args&&... args) const = 0;
            virtual impl_base* clone() const = 0;
        };

        template<typename F>
        class impl : public impl_base {
        public:
            impl(F f) : f(std::forward<F>(f)) {}
            virtual R invoke(Args&&... args) const override {
                return f(std::forward<Args>(args)...);
            }
            virtual impl_base* clone() const override {
                return new impl(f);
            }
        private:
            F f;
        };

        template<typename F>
        static void delete_impl(void* p) {
            delete static_cast<impl<F>*>(p);
        }

        impl_base* ptr;
        void (*deleter)(void*);
    };
}