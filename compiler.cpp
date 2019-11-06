#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// All instructions are encoded by 4 bytes
// Values obtained using arm-linux-gnueabi-objdump
namespace AssemblerInstructions {
uint32_t PushR4LR = 0xe92d4010;
uint32_t PopR4LR = 0xe8bd4010;
uint32_t BxLR = 0xe12fff1e;

uint32_t AddR0R1 = 0xe0800001;
uint32_t SubR0R1 = 0xe0400001;
uint32_t MulR0R1 = 0xe0000190;
uint32_t PushR0 = 0xe52d0004;

uint32_t LdrR0FromR0 = 0xe5900000;  // ldr r0, [r0]
uint32_t MovR4R0 = 0xe1a04000;
uint32_t BlxR4 = 0xe12fff34;
uint32_t PopR0 = 0xe49d0004;
uint32_t PopR1 = 0xe49d1004;
uint32_t PopR2 = 0xe49d2004;
uint32_t PopR3 = 0xe49d3004;

uint32_t MovwR0 = 0xe3000000;
uint32_t MovtR0 = 0xe3400000;
};  // namespace AssemblerInstructions

class Parser {
 public:
  Parser(const std::map<std::string, int>& extern_values)
      : extern_values_(extern_values) {}

  std::vector<uint32_t> GetInstructionsBuffer(const std::string& expression) {
    expression_ = expression;
    index_ = 0;
    out_.clear();
    PushInstruction(AssemblerInstructions::PushR4LR);
    Parse();
    PushInstruction(AssemblerInstructions::PopR0);
    PushInstruction(AssemblerInstructions::PopR4LR);
    PushInstruction(AssemblerInstructions::BxLR);
    return out_;
  }

  // Split expression into smaller parts
  // Then get their sum or distinction
  void Parse() {
    ParseProduct();
    while (true) {
      if (index_ == expression_.size()) {
        return;
      }

      char symbol = expression_[index_];
      index_++;
      if (symbol != '+' && symbol != '-') {
        index_--;
        return;
      }

      ParseProduct();
      PushInstruction(AssemblerInstructions::PopR1);
      PushInstruction(AssemblerInstructions::PopR0);
      if (symbol == '+') {
        PushInstruction(AssemblerInstructions::AddR0R1);
      } else if (symbol == '-') {
        PushInstruction(AssemblerInstructions::SubR0R1);
      }
      PushInstruction(AssemblerInstructions::PushR0);
    }
  }

  // Split parts of expression into tokens
  // Then get their product
  void ParseProduct() {
    ParseToken();
    while (true) {
      if (index_ == expression_.size()) {
        return;
      }

      char symbol = expression_[index_];
      index_++;
      if (symbol != '*') {
        index_--;
        return;
      }

      ParseToken();
      PushInstruction(AssemblerInstructions::PopR1);
      PushInstruction(AssemblerInstructions::PopR0);
      PushInstruction(AssemblerInstructions::MulR0R1);
      PushInstruction(AssemblerInstructions::PushR0);
    }
  }

  void ParseToken() {
    if (index_ == expression_.size()) {
      return;
    }

    char symbol = expression_[index_];
    index_++;

    if (symbol == '-') {
      ParseToken();
      MoveValueToR0(-1);
      PushInstruction(AssemblerInstructions::PushR0);
      PushInstruction(AssemblerInstructions::PopR1);  // write -1 to R1
      PushInstruction(AssemblerInstructions::PopR0);  // get the next value
      PushInstruction(AssemblerInstructions::MulR0R1);
      PushInstruction(AssemblerInstructions::PushR0);  // push to stack
      return;
    }

    if (symbol == '(') {
      Parse();
      index_++;  // read ')'
      return;
    }

    if (isdigit(symbol)) {
      std::string number_str;
      while (isdigit(symbol)) {
        number_str += symbol;
        symbol = expression_[index_];
        index_++;
      }
      index_--;
      int value = std::stoi(number_str);
      MoveValueToR0(value);
      PushInstruction(AssemblerInstructions::PushR0);
      return;
    }

    // if it is a named instance
    std::string name;
    while (isalpha(symbol)) {
      name += symbol;
      symbol = expression_[index_];
      index_++;
    }

    if (symbol == EOF) {
      MoveValueToR0(extern_values_.at(name));
      PushInstruction(AssemblerInstructions::LdrR0FromR0);
      PushInstruction(AssemblerInstructions::PushR0);
      return;
    }

    // if it is external variable
    if (symbol != '(') {
      index_--;
      MoveValueToR0(extern_values_.at(name));
      PushInstruction(AssemblerInstructions::LdrR0FromR0);
      PushInstruction(AssemblerInstructions::PushR0);
      return;
    }

    // it is external function
    ParseFunction(name);
  }

 private:
  void PushInstruction(const uint32_t& instruction) {
    out_.push_back(instruction);
  }

  // We cannot move any constant to register because of mov restriction
  // One of solutions is to load constant from memory
  // And another is to use ARMv7 movw and movt

  // movw (move wide) move a 16-bit constant into a register,
  // implicitly zeroing the top 16 bits of the target register

  // movt (move top) move a 16-bit constant into the top half
  // of a given register without altering the bottom 16 bits.
  void MoveValueToR0(int x) {
    uint32_t lower_bytes = x & ((1 << 16) - 1);
    uint32_t upper_bytes = x >> 16;

    uint32_t first_instruction = AssemblerInstructions::MovwR0;
    // write constant to 0-2 and 4 bytes of instruction
    first_instruction |=
        lower_bytes & ((1 << 12) - 1);  // lower 3 bytes of constant
    first_instruction |= (lower_bytes & (((1 << 4) - 1) << 12))
                         << 4;  // upper byte of constant

    uint32_t second_instruction = AssemblerInstructions::MovtR0;
    second_instruction |= upper_bytes & ((1 << 12) - 1);
    second_instruction |= (upper_bytes & (((1 << 4) - 1) << 12)) << 4;

    PushInstruction(first_instruction);
    PushInstruction(second_instruction);
  }

  void ParseFunction(const std::string& name) {
    char symbol;
    size_t num_arguments = 0;
    do {
      symbol = expression_[index_];
      index_++;
      if (symbol == ')') {
        break;
      }
      index_--;
      Parse();
      symbol = expression_[index_];
      index_++;
      num_arguments++;
    } while (symbol == ',');

    MoveValueToR0(extern_values_.at(name));
    PushInstruction(AssemblerInstructions::MovR4R0);
    uint32_t pop_instructions[4] = {
        AssemblerInstructions::PopR0, AssemblerInstructions::PopR1,
        AssemblerInstructions::PopR2, AssemblerInstructions::PopR3};

    for (size_t argument = 4; argument > 0; argument--) {
      if (num_arguments >= argument) {
        PushInstruction(pop_instructions[argument - 1]);
      }
    }
    PushInstruction(AssemblerInstructions::BlxR4);
    PushInstruction(AssemblerInstructions::PushR0);
  }

  size_t index_;
  std::string expression_;
  std::map<std::string, int> extern_values_;
  std::vector<uint32_t> out_;
};

extern "C" struct symbol_t {
  const char* name;
  void* pointer;
};

// C code can use this function
extern "C" void jit_compile_expression_to_arm(const char* expression,
                                              const symbol_t* externs,
                                              void* out_buffer) {
  std::string formatted_expression;
  for (size_t index = 0; expression[index] != '\0'; ++index) {
    if (expression[index] != ' ') {
      formatted_expression += expression[index];
    }
  }

  std::map<std::string, int> extern_values;
  for (size_t index = 0;
       externs[index].name != nullptr || externs[index].pointer != nullptr;
       ++index) {
    extern_values[std::string(externs[index].name)] =
        reinterpret_cast<int>(externs[index].pointer);
  }

  Parser parser(extern_values);
  std::vector<uint32_t> out =
      parser.GetInstructionsBuffer(formatted_expression);

  uint32_t* ptr = static_cast<uint32_t*>(out_buffer);
  for (size_t index = 0; index < out.size(); ++index) {
    *ptr = out[index];
    ++ptr;
  }
}