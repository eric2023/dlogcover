
    ## 单元测试规范
    ### 规则
    在理解源代码接口逻辑后，你应该始终遵循以下原则：
    - 禁止修改源文件中的函数实现！禁止修改源文件中的函数实现！禁止修改源文件中的函数实现！
    - 保持至少80%的单元测试覆盖率
    - 尽量仅针对公共方法编写单元测试。
    - 避免直接测试外部因素，如三方API接口、模块或系统环境；使用 cpp-stub-ext 打桩模拟界面UI和文件I/O等操作，保证测试环境独立性
    - 使用GTest作为单元测试框架
    - 单元测试应该专注于测试一个特定的组件或功能模块
    - 单元测试代码生成到一个文件当中
    - 只需要生成相关的单元测试代码
    
    ### 测试用例要求:
    - 包含基本功能测试
    - 包含边界条件测试
    - 包含错误处理测试
    - 包含内存管理测试
    
    ### 额外条件：
    - 对于Qt特有的功能(如信号和槽),使用 QSignalSpy 进行测试.
    - 使用 mock 对象来模拟用户交互和本地环境，比如模拟文件操作,避免依赖实际的文件系统。
    
    ## cpp-stub-ext使用方式
    ### 为常规函数打桩，使用参数
    ```cpp
    #include "stubext.h"
    using namespace stub_ext;


​    
​    int test(){return 0;}
​    int ret_test(int a){return a;}
​    static int add_test(int a ,int b){return a + b;}
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(&test,[&dd](){dd = 9;return dd;});
​        st.set_lamda(&ret_test,[&dd](int a){dd = 9 + a;return dd;});
​        st.set_lamda(&add_test,[&dd](int a, int b){dd = 9 + a - b;return dd;});
​        int r1 = test();
​        int r2 = ret_test(2);
​        int r2 = add_test(1,2);
​        return 0;
​    }
​    ```
​    
​    ### 为常规函数打桩，不使用参数
​    ```cpp
​    #include "stubext.h"
​    using namespace stub_ext;


​    
​    int test(){return 0;}
​    int ret_test(int a){return a;}
​    static int add_test(int a ,int b){return a + b;}
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(&test,[&dd](){dd = 9;return dd;});
​        st.set_lamda(&ret_test,[&dd](){dd = 9;return dd;});
​        st.set_lamda(&add_test,[&dd](){dd = 9;return dd;});
​        int r1 = test();
​        int r2 = ret_test(2);
​        int r2 = add_test(1,2);
​        return 0;
​    }
​    ```
​    
​    ### 为成员函数打桩，使用参数
​    ```cpp
​    #include "stubext.h"
​    using namespace stub_ext;


​    
​    class Test
​    {
​    public:
​        int test(){return m;}
​        int ret_test(int a) const {return m + a;}
​        int add_test(int a ,int b) {return m + a + b;}
​    private:
​        int m = 1;
​    };


​    
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(ADDR(Test,test),[&dd](Test *t){dd = 5; return dd;});
​        st.set_lamda(ADDR(Test,ret_test),[&dd](Test*,int a){dd = 5 + a; return dd;});
​        st.set_lamda(ADDR(Test,add_test),[&dd](Test*,int a, int b){dd = 5 + a - b; return dd;});
​        
​        Test t;
​        int r1 = t.test();
​        int r2 = t.ret_test(2);
​        int r2 = t.add_test(1,2);
​        return 0;
​    }
​    ```
​    
    ### 为成员函数打桩，不使用参数
    ```cpp
    #include "stubext.h"
    using namespace stub_ext;


​    
​    class Test
​    {
​    public:
​        int test(){return m;}
​        int ret_test(int a) const {return m + a;}
​        int add_test(int a ,int b) {return m + a + b;}
​    private:
​        int m = 1;
​    };


​    
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(ADDR(Test,test),[&dd](){dd = 5; return dd;});
​        st.set_lamda(ADDR(Test,ret_test),[&dd](){dd = 5; return dd;});
​        st.set_lamda(ADDR(Test,add_test),[&dd](){dd = 5; return dd;});
​        
​        Test t;
​        int r1 = t.test();
​        int r2 = t.ret_test(2);
​        int r2 = t.add_test(1,2);
​        return 0;
​    }
​    ```
​    
    ### 为类中的静态函数打桩，使用参数
    ```cpp
    #include "stubext.h"
    using namespace stub_ext;


​    
​    class Test
​    {
​    public:
​        static int sub_test(int a ,int b) {return a - b;}
​    };


​    
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(ADDR(Test,sub_test),[&dd](int a, int b){dd = 5 - a - b; return dd;});
​        
​        Test t;
​        int r1 = t.sub_test(1,2);
​        int r2 = Test::sub_test(1,2);
​        return 0;
​    }
​    ```
​    
    ### 为类中的静态函数打桩，不使用参数
    ```cpp
    #include "stubext.h"
    using namespace stub_ext;


​    
​    class Test
​    {
​    public:
​        static int sub_test(int a ,int b) {return a - b;}
​    };


​    
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(ADDR(Test,sub_test),[&dd](){dd = 5; return dd;});
​        
​        Test t;
​        int r1 = t.sub_test(1,2);
​        int r2 = Test::sub_test(1,2);
​        return 0;
​    }
​    ```
​    
    ### 为虚函数打桩
    使用`VADDR()`便捷地获取虚函数地址。
    ```cpp
    #include "stubext.h"
    using namespace stub_ext;


​    
​    class Test
​    {
​    public:
​        virtual int test() {return 0;}
​    };
​    class TestB : public Test
​    {
​    public:
​        virtual int test() {return 1;}
​    };
​    static int foo_stub()
​    {
​        return 9;
​    }


​    
​    int main(int argc, char *argv[])
​    {
​        int dd = -1;
​        StubExt st;
​        st.set_lamda(VADDR(Test,test),[&dd](){dd = 5; return dd;});
​        st.set(VADDR(TestB,test),&foo_stub);
​        
​        Test *t1 = new Test;
​        Test *t2 = new TestB;
​        int r1 = t1->test();
​        int r2 = t2->test();
​        delete t1;
​        delete t2;
​        return 0;
​    }
​    ```