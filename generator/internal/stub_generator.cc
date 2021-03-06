// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "generator/internal/stub_generator.h"
#include "google/cloud/internal/absl_str_cat_quiet.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_split.h"
#include "generator/internal/codegen_utils.h"
#include "generator/internal/descriptor_utils.h"
#include "generator/internal/predicate_utils.h"
#include "generator/internal/printer.h"
#include <google/api/client.pb.h>
#include <google/protobuf/descriptor.h>

namespace google {
namespace cloud {
namespace generator_internal {

StubGenerator::StubGenerator(
    google::protobuf::ServiceDescriptor const* service_descriptor,
    VarsDictionary service_vars,
    std::map<std::string, VarsDictionary> service_method_vars,
    google::protobuf::compiler::GeneratorContext* context)
    : ServiceCodeGenerator("stub_header_path", "stub_cc_path",
                           service_descriptor, std::move(service_vars),
                           std::move(service_method_vars), context) {}

Status StubGenerator::GenerateHeader() {
  HeaderPrint(CopyrightLicenseFileHeader());
  HeaderPrint(  // clang-format off
    "// Generated by the Codegen C++ plugin.\n"
    "// If you make any local changes, they will be lost.\n"
    "// source: $proto_file_name$\n"
    "#ifndef $header_include_guard$\n"
    "#define $header_include_guard$\n"
    "\n");
  // clang-format on

  // includes
  HeaderLocalIncludes({HasStreamingReadMethod()
                           ? "google/cloud/internal/streaming_read_rpc.h"
                           : "",
                       "google/cloud/status_or.h", "google/cloud/version.h"});
  HeaderSystemIncludes(
      {vars("proto_grpc_header_path"),
       HasLongrunningMethod() ? "google/longrunning/operations.grpc.pb.h" : "",
       "memory"});
  HeaderPrint("\n");

  auto result = HeaderOpenNamespaces(NamespaceType::kInternal);
  if (!result.ok()) return result;

  // Abstract interface Stub base class
  HeaderPrint(  // clang-format off
    "class $stub_class_name$ {\n"
    " public:\n"
    "  virtual ~$stub_class_name$() = 0;\n"
    "\n");
  // clang-format on

  for (auto const& method : methods()) {
    HeaderPrintMethod(
        method,
        {MethodPattern(
             {{IsResponseTypeEmpty,
               // clang-format off
    "  virtual Status $method_name$(\n",
    "  virtual StatusOr<$response_type$> $method_name$(\n"},
   {"    grpc::ClientContext& context,\n"
    "    $request_type$ const& request) = 0;\n"
               // clang-format on
               "\n"}},
             IsNonStreaming),
         MethodPattern(
             {// clang-format off
   {"  virtual std::unique_ptr<internal::StreamingReadRpc<$response_type$>>\n"
    "  $method_name$(\n"
    "    grpc::ClientContext& context,\n"
    "    $request_type$ const& request) = 0;\n"
    "\n"}},
             // clang-format on
             IsStreamingRead)},
        __FILE__, __LINE__);
  }

  // long running operation support methods
  if (HasLongrunningMethod()) {
    HeaderPrint(  // clang-format off
    "  /// Poll a long-running operation.\n"
    "  virtual StatusOr<google::longrunning::Operation> GetOperation(\n"
    "      grpc::ClientContext& client_context,\n"
    "      google::longrunning::GetOperationRequest const& request) = 0;\n"
    "\n"
    "  /// Cancel a long-running operation.\n"
    "  virtual Status CancelOperation(\n"
    "      grpc::ClientContext& client_context,\n"
    "      google::longrunning::CancelOperationRequest const& request) = 0;\n"
    "\n");
    // clang-format on
  }
  // close abstract interface Stub base class
  HeaderPrint(  // clang-format off
    "};\n\n");
  // clang-format on

  // default stub class
  HeaderPrint(  // clang-format off
    "class Default$stub_class_name$ : public $stub_class_name$ {\n"
    " public:\n");
  if (HasLongrunningMethod()) {
    HeaderPrint(  // clang-format off
    "  Default$stub_class_name$(\n"
    "      std::unique_ptr<$grpc_stub_fqn$::StubInterface> grpc_stub,\n"
    "      std::unique_ptr<google::longrunning::Operations::StubInterface> "
    "operations)\n"
    "      : grpc_stub_(std::move(grpc_stub)),\n"
    "        operations_(std::move(operations)) {}\n\n");
    // clang-format on
  } else {
    HeaderPrint(  // clang-format off
    "  explicit Default$stub_class_name$(\n"
    "      std::unique_ptr<$grpc_stub_fqn$::StubInterface> grpc_stub)\n"
    "      : grpc_stub_(std::move(grpc_stub)) {}\n\n");
    // clang-format on
  }

  for (auto const& method : methods()) {
    // emit methods
    HeaderPrintMethod(
        method,
        {MethodPattern({{IsResponseTypeEmpty,
                         // clang-format off
    "  Status\n",
    "  StatusOr<$response_type$>\n"},
    {"  $method_name$(\n"
    "    grpc::ClientContext& client_context,\n"
    "    $request_type$ const& request) override;\n"
    "\n"}},
                       // clang-format on
                       IsNonStreaming),
         MethodPattern(
             {// clang-format off
   {"  std::unique_ptr<internal::StreamingReadRpc<$response_type$>>\n"
    "  $method_name$(\n"
    "    grpc::ClientContext& client_context,\n"
    "    $request_type$ const& request) override;\n"
    "\n"}},
             // clang-format on
             IsStreamingRead)},
        __FILE__, __LINE__);
  }

  if (HasLongrunningMethod()) {
    // long running operation support methods
    HeaderPrint(  // clang-format off
    "  /// Poll a long-running operation.\n"
    "  StatusOr<google::longrunning::Operation> GetOperation(\n"
    "      grpc::ClientContext& client_context,\n"
    "      google::longrunning::GetOperationRequest const& request) override;\n"
    "\n"
    "  /// Cancel a long-running operation.\n"
    "  Status CancelOperation(\n"
    "      grpc::ClientContext& client_context,\n"
    "      google::longrunning::CancelOperationRequest const& request) override;\n"
    "\n");
    // clang-format on
  }

  // private members and close default stub class defintion
  HeaderPrint(  // clang-format off
    " private:\n"
    "  std::unique_ptr<$grpc_stub_fqn$::StubInterface> grpc_stub_;\n"
  );
  if (HasLongrunningMethod()) {
    HeaderPrint(  // clang-format off
    "  std::unique_ptr<google::longrunning::Operations::StubInterface> operations_;\n");
    // clang-format on
  }
  HeaderPrint(  // clang-format off
    "};\n\n");
  // clang-format on

  HeaderCloseNamespaces();
  // close header guard
  HeaderPrint(  // clang-format off
      "#endif  // $header_include_guard$\n");
  // clang-format on
  return {};
}

Status StubGenerator::GenerateCc() {
  CcPrint(CopyrightLicenseFileHeader());
  CcPrint(  // clang-format off
    "// Generated by the Codegen C++ plugin.\n"
    "// If you make any local changes, they will be lost.\n"
    "// source: $proto_file_name$\n\n");
  // clang-format on

  // includes
  CcLocalIncludes({vars("stub_header_path"),
                   HasStreamingReadMethod() ? "absl/memory/memory.h" : "",
                   "google/cloud/grpc_error_delegate.h",
                   "google/cloud/status_or.h"});
  CcSystemIncludes(
      {vars("proto_grpc_header_path"),
       HasLongrunningMethod() ? "google/longrunning/operations.grpc.pb.h" : "",
       "memory"});
  CcPrint("\n");

  auto result = CcOpenNamespaces(NamespaceType::kInternal);
  if (!result.ok()) return result;

  CcPrint(  // clang-format off
    "$stub_class_name$::~$stub_class_name$() = default;\n\n");
  // clang-format on

  // default stub class member methods
  for (auto const& method : methods()) {
    CcPrintMethod(
        method,
        {MethodPattern(
             {{IsResponseTypeEmpty,
               // clang-format off
    "Status\n",
    "StatusOr<$response_type$>\n"},
    {"Default$stub_class_name$::$method_name$(\n"
    "  grpc::ClientContext& client_context,\n"
    "  $request_type$ const& request) {\n"
    "    $response_type$ response;\n"
    "    auto status =\n"
    "        grpc_stub_->$method_name$(&client_context, request, &response);\n"
    "    if (!status.ok()) {\n"
    "      return google::cloud::MakeStatusFromRpcError(status);\n"
    "    }\n"},
   {IsResponseTypeEmpty,
    "    return google::cloud::Status();\n",
    "    return response;\n"},
   {"}\n"
    "\n"}},
             // clang-format on
             IsNonStreaming),
         MethodPattern(
             {// clang-format off
   {"std::unique_ptr<internal::StreamingReadRpc<$response_type$>>\n"
    "Default$stub_class_name$::$method_name$(\n"
    "    grpc::ClientContext&,\n"
    "    $request_type$ const& request) {\n"
    "  auto context = absl::make_unique<grpc::ClientContext>();\n"
    "  auto stream = grpc_stub_->TailLogEntries(context.get(), request);\n"
    "  return absl::make_unique<internal::StreamingReadRpcImpl<\n"
    "      ::google::test::admin::database::v1::TailLogEntriesResponse>>(\n"
    "      std::move(context), std::move(stream));\n"
    "}\n\n"}},
             // clang-format on
             IsStreamingRead)},
        __FILE__, __LINE__);
  }

  if (HasLongrunningMethod()) {
    // long running operation support methods
    CcPrint(  // clang-format off
    "/// Poll a long-running operation.\n"
    "StatusOr<google::longrunning::Operation>\n"
    "Default$stub_class_name$::GetOperation(\n"
    "    grpc::ClientContext& client_context,\n"
    "    google::longrunning::GetOperationRequest const& request) {\n"
    "  google::longrunning::Operation response;\n"
    "  grpc::Status status =\n"
    "      operations_->GetOperation(&client_context, request, &response);\n"
    "  if (!status.ok()) {\n"
    "    return google::cloud::MakeStatusFromRpcError(status);\n"
    "  }\n"
    "  return response;\n"
    "}\n"
    "/// Cancel a long-running operation.\n"
    "Status Default$stub_class_name$::CancelOperation(\n"
    "    grpc::ClientContext& client_context,\n"
    "    google::longrunning::CancelOperationRequest const& request) {\n"
    "  google::protobuf::Empty response;\n"
    "  grpc::Status status =\n"
    "      operations_->CancelOperation(&client_context, request, &response);\n"
    "  if (!status.ok()) {\n"
    "    return google::cloud::MakeStatusFromRpcError(status);\n"
    "  }\n"
    "  return google::cloud::Status();\n"
    "}\n");
    // clang-format on
  }

  CcCloseNamespaces();
  return {};
}

}  // namespace generator_internal
}  // namespace cloud
}  // namespace google
