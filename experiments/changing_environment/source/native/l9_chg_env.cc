// This is the main function for the NATIVE version of this project.

#include <iostream>

#include "config/command_line.h"
#include "config/ArgManager.h"

#include "../l9_chg_env-config.h"
#include "../Experiment.h"

int main(int argc, char* argv[])
{
  // Read configs.
  std::string config_fname = "configs.cfg";
  auto args = emp::cl::ArgManager(argc, argv);
  L9ChgEnvConfig config;
  config.Read(config_fname);

  if (args.ProcessConfigOptions(config, std::cout, config_fname, "../l9l9_chg_env-config.h") == false)
    exit(0);
  if (args.TestUnknown() == false)
    exit(0);

  std::cout << "==============================" << std::endl;
  std::cout << "|    How am I configured?    |" << std::endl;
  std::cout << "==============================" << std::endl;
  config.Write(std::cout);
  std::cout << "==============================\n"
            << std::endl;

  Experiment e(config);
  e.Run();
}
