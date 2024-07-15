#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <vtkm/Types.h>

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

  bool GetArgAs(const std::string& argNm, vtkm::FloatDefault& val) const;
  bool GetArgAs(const std::string& argNm, vtkm::Id& val) const;

  bool GetArgAs(const std::string& argNm, vtkm::Vec2f_32& vec) const;
  bool GetArgAs(const std::string& argNm, vtkm::Vec3f_32& vec) const;

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


bool
CommandLineArgParser::GetArgAs(const std::string& argNm, vtkm::FloatDefault& val) const
{
  const auto& it = this->Args.find(argNm);
  if (it != this->Args.end())
  {
    const auto& params = it->second;
    if (params.size() == 1)
    {
      val = std::stof(params[0]);
      return true;
    }
  }

  return false;
}

bool
CommandLineArgParser::GetArgAs(const std::string& argNm, vtkm::Vec2f_32& vec) const
{
  const auto& it = this->Args.find(argNm);

  if (it != this->Args.end())
  {
    const auto& params = it->second;
    if (params.size() == 1)
    {
      vtkm::FloatDefault val = std::stof(params[0]);
      vec[0] = val;
      vec[1] = val;
      return true;
    }
    else if (params.size() == 2)
    {
      vec[0] = std::stof(params[0]);
      vec[1] = std::stof(params[1]);
      return true;
    }
  }

  return false;
}

bool
CommandLineArgParser::GetArgAs(const std::string& argNm, vtkm::Vec3f_32& vec) const
{
  const auto& it = this->Args.find(argNm);

  if (it != this->Args.end())
  {
    const auto& params = it->second;

    if (params.size() == 1)
    {
      vtkm::FloatDefault val = std::stof(params[0]);
      vec[0] = val;
      vec[1] = val;
      vec[2] = val;
      return true;
    }
    else if (params.size() == 3)
    {
      vec[0] = std::stof(params[0]);
      vec[1] = std::stof(params[1]);
      vec[2] = std::stof(params[2]);
      return true;
    }
  }

  return false;
}

}} //namespace xenia::utils
