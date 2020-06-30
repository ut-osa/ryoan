//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "native_client/src/trusted/service_runtime/trusted_exec.h"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include <python3.5m/Python.h>
using namespace clang;
using namespace clang::driver;


template <class T>
using Vec = SmallVector<T, 16>;

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char *Argv0) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *MainAddr = (void*) (intptr_t) GetExecutablePath;
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

static llvm::ExecutionEngine *
createExecutionEngine(std::unique_ptr<llvm::Module> M, std::string *ErrorStr) {
  return llvm::EngineBuilder(std::move(M))
      .setEngineKind(llvm::EngineKind::JIT)
      .setErrorStr(ErrorStr)
      .create();
}

void add_module(PyObject *(*init)(void), const char *module_name, const char *fullname,
                const char *cpath)
{
  PyObject *mod = init();
  PyObject *name = PyUnicode_FromString(fullname);
  PyObject *path = PyUnicode_FromString(cpath);
  PyModuleDef *def;

  /* Remember pointer to module init function. */
  def = PyModule_GetDef(mod);
  def->m_base.m_init = init;
  if (_PyImport_FixupExtensionObject(mod, name, name) < 0) {
    printf("Fixup failed!\n");
    Py_DECREF(name);
  }

  if (PyModule_AddObject(mod, "__file__", path) < 0)
    PyErr_Clear(); /* Not important enough to report */
  else
    Py_INCREF(path);

  Py_DECREF(mod);
  Py_DECREF(path);
  Py_DECREF(name);
}

std::vector<llvm::ExecutionEngine *> jitted_code;
int Execute(std::unique_ptr<llvm::Module> Mod, const char *init_func,
                  const char *module_name, const char *fullname, const char *path) {
  llvm::Module &M = *Mod;
  std::string Error;
  llvm::ExecutionEngine *EE = createExecutionEngine(std::move(Mod), &Error);
  if (!EE) {
    llvm::errs() << "unable to make execution engine: " << Error << "\n";
    return -1;
  }

  // FIXME: Support passing arguments.
  std::vector<std::string> Args;
  Args.push_back(M.getModuleIdentifier());

  EE->finalizeObject();
  PyObject *(*p0)(void) =
    (PyObject *(*)(void))EE->getFunctionAddress(init_func);
  if (!p0) {
     fprintf(stderr, "No module init function!\n");
     fflush(stderr);
  }

  jitted_code.push_back(EE);

  add_module(p0, module_name, fullname, path);
  return 0;
}

static int compile(Vec<const char *> &Args, const char *module_name, const char *full_name, const char *path) {

  char init_func[256];

  int ret;
  std::string Path = GetExecutablePath(Args[0]);
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  TextDiagnosticPrinter *DiagClient =
    new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

  snprintf(init_func, 256, "PyInit_%s", module_name);

  static bool inited = false;
  if (!inited) {
     llvm::InitializeNativeTargetAsmPrinter();
     llvm::InitializeNativeTarget();
     inited = true;
  }

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

  // Use ELF on windows for now.
  std::string TripleStr = llvm::sys::getProcessTriple();
  llvm::Triple T(TripleStr);
  if (T.isOSBinFormatCOFF())
    T.setObjectFormat(llvm::Triple::ELF);

  Driver TheDriver(Path, T.str(), Diags);
  TheDriver.setTitle("clang interpreter");
  TheDriver.setCheckInputsExist(false);

  Args.push_back("-fsyntax-only");
  std::unique_ptr<Compilation> C(TheDriver.BuildCompilation(Args));
  if (!C)
    return -1;

  // FIXME: This is copied from ASTUnit.cpp; simplify and eliminate.

  // We expect to get back exactly one command job, if we didn't something
  // failed. Extract that job from the compilation.
  const driver::JobList &Jobs = C->getJobs();
  if (Jobs.size() != 1 || !isa<driver::Command>(*Jobs.begin())) {
    SmallString<256> Msg;
    llvm::raw_svector_ostream OS(Msg);
    Jobs.Print(OS, "; ", true);
    Diags.Report(diag::err_fe_expected_compiler_job) << OS.str();
    return -1;
  }

  const driver::Command &Cmd = cast<driver::Command>(*Jobs.begin());
  if (llvm::StringRef(Cmd.getCreator().getName()) != "clang") {
    Diags.Report(diag::err_fe_expected_clang_command);
    return -1;
  }

  // Initialize a compiler invocation object from the clang (-cc1) arguments.
  const driver::ArgStringList &CCArgs = Cmd.getArguments();
  CompilerInvocation *CI = new CompilerInvocation;
  CompilerInvocation::CreateFromArgs(*CI,
                                     const_cast<const char **>(CCArgs.data()),
                                     const_cast<const char **>(CCArgs.data()) +
                                       CCArgs.size(),
                                     Diags);

  // Show the invocation, with -v.
  if (CI->getHeaderSearchOpts().Verbose) {
    llvm::errs() << "clang invocation:\n";
    Jobs.Print(llvm::errs(), "\n", true);
    llvm::errs() << "\n";
  }

  // FIXME: This is copied from cc1_main.cpp; simplify and eliminate.

  // Create a compiler instance to handle the actual work.
  CompilerInstance Clang;
  Clang.setInvocation(CI);

  // Create the compilers actual diagnostics engine.
  Clang.createDiagnostics();
  if (!Clang.hasDiagnostics())
    return -1;

  // Infer the builtin include path if unspecified.
  /*
  if (Clang.getHeaderSearchOpts().UseBuiltinIncludes &&
      Clang.getHeaderSearchOpts().ResourceDir.empty())
    Clang.getHeaderSearchOpts().ResourceDir =
      CompilerInvocation::GetResourcesPath(argv[0], MainAddr);
      */

  // Create and execute the frontend to generate an LLVM bitcode module.
  std::unique_ptr<CodeGenAction> Act(new EmitLLVMOnlyAction());
  if (!Clang.ExecuteAction(*Act))
    return -1;

  if (std::unique_ptr<llvm::Module> Module = Act->takeModule())
    ret = Execute(std::move(Module), init_func, module_name, full_name, path);

  // Shutdown.

  //llvm::llvm_shutdown();
  return ret;
}

/* find the first string in the range  [first, last) which matches the prefix
 * str
 */
template <class T>
T find_str (T first, T last, const char* str) {
   unsigned len = strlen(str);
   return std::find_if(first, last, [&](const char *a) {
         return strncmp(str, a, len) == 0;
   });
}

static inline
void remove_ops(Vec<const char *> &argv, const Vec<const char*> &bad_ops,
      unsigned trailing)
{
   unsigned shift = trailing + 1;
   for (const auto &op : bad_ops) {
      auto it = find_str(argv.begin(), argv.end(), op);
      while (it < argv.end() - trailing) {
         it = argv.erase(it, it + shift);
         it = find_str(it, argv.end(), op);
      }
   }
}

static void filter_ops(Vec<const char *> &argv)
{
   static const Vec<const char*> bad_ops = {
      "-msahf", "-mno-movbe", "-mno-abm", "-mno-lwp", "-mno-hle",
      "-mno-prefetchwt1", "-mno-clflushopt", "-mno-avx512ifma",
      "-mno-avx512vbmi", "-mno-clwb", "-mno-pcommit", "-mno-mwaitx",
      "-shared", "-l", "-L", "-m", "-Wl,-rpath,/usr/lib",
   };

   static const Vec<const char *> bad_arg_ops = {
      "-o", "--param",
   };

   remove_ops(argv, bad_ops, 0);
   remove_ops(argv, bad_arg_ops, 1);
}

/* returns the name of the binary (-o)
 */
static const char* construct_argv(char *str, Vec<const char *> &argv)
{
   static const Vec<const char *> extra_args = {
      SYS_INCLUDES,
      "-Wno-pointer-bool-conversion",
      "-Wno-absolute-value",
      "-D__CLANG_JIT__",
   };

   const char *out_path = NULL;

   for (char *p = strtok(str, " \n\r\t"); p; p = strtok(NULL, " \n\r\t"))
      argv.push_back(p);

   /* have to insert our args after the "compiler name" */
   argv.insert(argv.begin() + 1, extra_args.begin(), extra_args.end());

   /* need to find this first as filter_ops may remove -o */
   auto it = find_str(argv.begin(), argv.end(), "-o");
   if (it < argv.end() - 1)
      out_path = *(it + 1);

   filter_ops(argv);
   return out_path;
}

/* Return the number of arguments of the application command line */
static PyObject*
compile_cpp(PyObject *, PyObject *args)
{
   const char *const_cmd, *module_name, *full_name;
   const char *out_path;
   char *cmd;
   PyObject *res = NULL;
   Vec<const char*> argv;

   if(!PyArg_ParseTuple(args, "sss", &const_cmd, &module_name, &full_name))
      return NULL;

   cmd = (char *)malloc(strlen(const_cmd) + 1);
   if (!cmd) {
      PyErr_SetString(PyExc_RuntimeError, "malloc failed");
      return NULL;
   }

   out_path = construct_argv(strcpy(cmd, const_cmd), argv);
   if (!out_path) {
      PyErr_SetString(PyExc_RuntimeError, "Compiler arguments should have -o <outfile>");
      return NULL;
   }

   if (compile(argv, module_name, full_name, out_path))
      PyErr_SetString(PyExc_RuntimeError, "Compilation failed!");
   else
      res = PyTuple_Pack(3, PyBytes_FromString("stdout"), PyBytes_FromString("stderr"), PyLong_FromLong(0));

   free(cmd);
   return res;
}

static PyMethodDef Methods[] = {
   {"compile", compile_cpp, METH_VARARGS,
      "Compile c++ code"},
   {NULL, NULL, 0, NULL}
};

static PyModuleDef CompilerModule = {
   PyModuleDef_HEAD_INIT, "clangjit", NULL, -1, Methods,
      NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_CompilerModule(void)
{
   return PyModule_Create(&CompilerModule);
}

int load_compiler_module(void)
{
   PyImport_AppendInittab("clangjit", PyInit_CompilerModule);
   return 0;
}

extern "C" {
/* mention these functions here to ensure they get linked in
 * (args/returntype doesn't matter */
void sgemm_(void);
}

void __force_link(void) __attribute__((used));
void __force_link(void) {
   sgemm_();
}
