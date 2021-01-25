cmake_minimum_required(VERSION 3.13.0)

include(CSharpUtilities)
project(RealSenseID_CSharp_Example LANGUAGES CSharp)

set(CMAKE_CSharp_FLAGS "/platform:x64")

set(EXE_NAME rsid_csharp_example)
add_executable(
    ${EXE_NAME}
    App.config
    App.xaml
    App.xaml.cs
    MainWindow.xaml
    MainWindow.xaml.cs
    EnrollInput.xaml
    EnrollInput.xaml.cs
    DeleteUserInput.xaml
    DeleteUserInput.xaml.cs
    Utilities.cs    
    AuthSettingsInput.xaml
    AuthSettingsInput.xaml.cs
    Properties/AssemblyInfo.cs
    Properties/Resources.Designer.cs
    Properties/Resources.resx
    Properties/Settings.Designer.cs
    Properties/Settings.settings
    icon.ico)

# designer files
csharp_set_designer_cs_properties(Properties/AssemblyInfo.cs Properties/Resources.Designer.cs Properties/Resources.resx
                                  Properties/Settings.Designer.cs Properties/Settings.settings)

csharp_set_xaml_cs_properties(App.xaml App.xaml.cs MainWindow.xaml MainWindow.xaml.cs EnrollInput.xaml
                              EnrollInput.xaml.cs  AuthSettingsInput.xaml AuthSettingsInput.xaml.cs DeleteUserInput.xaml DeleteUserInput.xaml.cs)

# entry point
set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")


# cs project flags
set_property(TARGET ${EXE_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")
set_property(TARGET ${EXE_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
set_property(TARGET ${EXE_NAME} PROPERTY VS_GLOBAL_ApplicationIcon ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico)

# references
set_property(
    TARGET ${EXE_NAME}
    PROPERTY VS_DOTNET_REFERENCES
             "Microsoft.CSharp"
             "PresentationCore"
             "PresentationFramework"
             "System"
             "System.Core"
             "System.Data"
             "System.Data.DataSetExtensions"
             "System.Net.Http"
             "System.Xaml"
             "System.Xml"
             "System.Xml.Linq"
             "System.Configuration"
             "System.Management"
             "WindowsBase")

target_link_libraries(${EXE_NAME} PRIVATE rsid_dotnet rsid_signature_example)

set_property(TARGET ${EXE_NAME} PROPERTY FOLDER "examples")