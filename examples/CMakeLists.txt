# 示例目录CMakeLists.txt

# 安装示例配置文件
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dlogcover.json
    DESTINATION share/dlogcover/examples
)

# 目前没有示例代码需要编译，如有需要可在此添加 