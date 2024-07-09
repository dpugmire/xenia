#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace xenia
{
namespace utils
{

class CommandLineArgParser
{
public:
  CommandLineArgParser() {}
  CommandLineArgParser(int argc, char** argv)
  {
    this->SetArgs(argc, argv);
  }
  CommandLineArgParser(int argc, char** argv, const std::vector<std::string>& req)
    : RequiredArgs(req)
  {
    this->SetArgs(argc, argv);
    this->CheckRequired();
  }

  bool HasArg(const std::string& argNm) const
  {
    return this->Args.find(argNm) != this->Args.end();
  }

  std::vector<std::string> GetArg(const std::string& argNm) const
  {
    std::vector<std::string> res;
    auto it = this->Args.find(argNm);
    if (it != this->Args.end())
      res = it->second;

    return res;
  }

  bool CheckRequired() const
  {
    bool retVal = true;
    for (const auto& ra : this->RequiredArgs)
    {
      const auto& it = this->Args.find(ra);
      if (it == this->Args.end())
      {
        std::cerr<<"Error. Argument missing: "<<ra<<std::endl;
        retVal = false;
      }
    }

    return retVal;
  }

private:

  void SetArgs(int argc, char** argv)
  {
    this->Args.clear();
    std::string a0;
    std::vector<std::string> a1;

    for (int i = 1; i < argc; i++)
    {
      std::string tmp(argv[i]);
      if (tmp.find("--") != std::string::npos)
      {
        if (!a0.empty())
        {
          this->Args[a0] = a1;
          a1.clear();
        }
        a0 = tmp;
        continue;
      }
      else
        a1.push_back(tmp);
    }
    //last argument.
    if (!a0.empty())
      this->Args[a0] = a1;
  }

  std::vector<std::string> RequiredArgs;
  std::map<std::string, std::vector<std::string>> Args;
};

}} //namespace xenia::utils
