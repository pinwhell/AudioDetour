add_library(SimpleWidget STATIC SimpleWidget.cpp)
target_include_directories(SimpleWidget PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(SimpleWidget nana)

add_library(SoundDetour SHARED  SoundDetour.cpp Utility.cpp VTable.cpp MMDetours.cpp)
target_link_libraries(SoundDetour minhook fmt SimpleWidget comsuppw psapi dll-injector)