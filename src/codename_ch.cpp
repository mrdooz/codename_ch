// codename_ch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../system/System.hpp"
#include "../redux/DefaultRenderer.hpp"
#include "../redux/M2Renderer.hpp"
#include "../redux/ShadowRenderer.hpp"
#include "../redux/Dynamic.hpp"
#include "../redux/Dynamic2.hpp"
#include "../redux/Countdown.hpp"
#include "../redux/SpringTest.hpp"
#include "../redux/MarchingCubes.hpp"
#include "../redux/Particles.hpp"
#include "../system/Serializer.hpp"

namespace test = boost::unit_test;


void redirect_io_to_console() 
{
  // maximum mumber of lines the output console should have
  const WORD MAX_CONSOLE_LINES = 500;

  // allocate a console for this app
  AllocConsole();

  // set the screen buffer to be big enough to let us scroll text
  CONSOLE_SCREEN_BUFFER_INFO coninfo;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);

  coninfo.dwSize.Y = MAX_CONSOLE_LINES;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

  // redirect unbuffered STDOUT to the console
  *stdout = *_fdopen(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "w" );
  setvbuf( stdout, NULL, _IONBF, 0 );

  // redirect unbuffered STDIN to the console
  *stdin = *_fdopen( _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT), "r" );
  setvbuf( stdin, NULL, _IONBF, 0 );

  // redirect unbuffered STDERR to the console
  *stderr = *_fdopen(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT), "w" );
  setvbuf( stderr, NULL, _IONBF, 0 );

  // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
  // point to console as well
  std::ios::sync_with_stdio();
}


void string_id_test()
{
  typedef std::map<StringId, std::vector<int> > StringIdMap;
  StringIdMap map;
  StringId tjong_id("tjong");
  StringId bong_id("bong");

  map[StringId("tjong")].push_back(10);
  map[StringId("bong")].push_back(20);
  map[StringId("tjong")].push_back(20);
  BOOST_CHECK(map.size() == 2);
  BOOST_CHECK(map[tjong_id].size() == 2);
  BOOST_CHECK(map[bong_id].size() == 1);
}

test::test_suite* init_unit_test_suite(int, char* [])
{
  test::test_suite* suite = BOOST_TEST_SUITE("codename_ch test suite");
  suite->add( BOOST_TEST_CASE( &string_id_test ) );
  return suite;
}

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/ )
{
  redirect_io_to_console();

  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
  _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  //::boost::unit_test::unit_test_main(init_unit_test_suite, 0, 0);

  boost::shared_ptr<System> system(new System());
  system->init();

  boost::shared_ptr<EffectManager> effect_manager(new EffectManager(system->handle_manager()));
//  Renderable* renderer = new SpringTest(system, effect_manager);
  //Renderable* renderer = new M2Renderer(system, effect_manager);
  Renderable *renderer = new MarchingCubes(system, effect_manager);

  SCOPED_PROFILE("Loading");
  try {
    renderer->init();
  } catch (std::exception& e) {
    LOG_ERROR_LN("%s", e.what());
    return 0;
  }
  Profiler::instance().print();

  Serializer::instance().load("codename_ch.dat");

  system->run();
  system->close();

  Serializer::instance().save("codename_ch.dat");

  SAFE_DELETE(renderer);

  effect_manager.reset();
  system.reset();

  Profiler::close();
  StringIdTable::close();
  LogMgr::close();

  return 0;
}
