#include "codegen.hpp"
#include "ast.hpp"
void Codegen::flush()
{
    // Routine itoa
    text_prefix_ += "itoa:\n";
    text_prefix_ += "    push rbx\n";
    text_prefix_ += "    push rcx\n";
    text_prefix_ += "    mov rcx, itoa_buf + 20\n";
    text_prefix_ += "    mov rbx, 10\n";
    text_prefix_ += ".itoa_loop:\n";
    text_prefix_ += "    xor rdx, rdx\n";
    text_prefix_ += "    div rbx\n";
    text_prefix_ += "    add dl, '0'\n";
    text_prefix_ += "    dec rcx\n";
    text_prefix_ += "    mov [rcx], dl\n";
    text_prefix_ += "    test rax, rax\n";
    text_prefix_ += "    jnz .itoa_loop\n";
    text_prefix_ += "    mov rsi, rcx\n";
    text_prefix_ += "    mov rdx, itoa_buf + 20\n";
    text_prefix_ += "    sub rdx, rcx\n";
    text_prefix_ += "    pop rcx\n";
    text_prefix_ += "    pop rbx\n";
    text_prefix_ += "    ret\n";

    // Exit syscall
    text_ += "    mov rax, 60\n";
    text_ += "    xor rdi, rdi\n";
    text_ += "    syscall\n";
    data_ += "    itoa_buf times 21 db 0\n";
    data_ += "    __newline db 0x0a\n";
    data_ += "    true_msg db \"true\"\n";
    data_ += "    false_msg db \"false\"\n";
    out_ << "section .data\n";
    out_ << data_;
    out_ << "section .text\n";
    out_ << "global _start\n";
    out_ << text_prefix_;
    out_ << "_start:\n";
    out_ << text_;
}

Codegen::Codegen(const std::string& output_file)
{
    out_.open(output_file);
}

void Codegen::generate(ASTNode* node)
{
    if(auto n = dynamic_cast<Block*>(node))
    gen_block(n);
    else if(auto n = dynamic_cast<Assign*>(node))
    gen_assign(n);
    else if(auto n = dynamic_cast<Output*>(node))
    gen_output(n);
    else if(auto n = dynamic_cast<IfStmt*>(node))
    gen_if(n);
    else if(auto n = dynamic_cast<RepeatStmt*>(node))
    gen_repeat(n);
}

void Codegen::gen_block(Block* node)
{
    for(auto& s : node->statements)
    {
        generate(s.get());
    }
}

void Codegen::gen_assign(Assign* node)
{
if(auto n = dynamic_cast<StringLit*>(node->value.get()))
    {
        data_ += "    " + node->name + " db \"" + n->value + "\", 0\n";
        data_ += "    " + node->name + "_len equ $ - " + node->name + " - 1\n";
        string_vars_.insert(node->name);
    }
    else if(auto n = dynamic_cast<BoolLit*>(node->value.get()))
    {
        data_ += "    " + node->name + " dq " + (n->value ? "1" : "0") + "\n";
        bool_vars_.insert(node->name);
    }
    else if(auto n = dynamic_cast<NumberLit*>(node->value.get()))
    {
        data_ += "    " + node->name + " dq " + std::to_string((int)n->value) + "\n";
    }
}

void Codegen::gen_output(Output* node)
{
	auto emit_newline = [&]() {
		text_ += "    mov rax, 1\n";
		text_ += "    mov rdi, 1\n";
		text_ += "    mov rsi, __newline\n";
		text_ += "    mov rdx, 1\n";
		text_ += "    syscall\n";
	};
	if(auto n = dynamic_cast<StringLit*>(node->value.get()))
	{
		std::string lbl = "__str" + std::to_string(str_counter_++);
		data_ += "    " + lbl + " db \"" + n->value + "\", 0\n";
		data_ += "    " + lbl + "_len equ $ - " + lbl + " - 1\n";
		text_ += "    mov rax, 1\n";
		text_ += "    mov rdi, 1\n";
		text_ += "    mov rsi, " + lbl + "\n";
		text_ += "    mov rdx, " + lbl + "_len\n";
		text_ += "    syscall\n";
		emit_newline();
	}
	else if(auto n = dynamic_cast<Identifier*>(node->value.get()))
	{
		if(string_vars_.count(n->name))
		{
			text_ += "    mov rax, 1\n";
			text_ += "    mov rdi, 1\n";
			text_ += "    mov rsi, " + n->name + "\n";
			text_ += "    mov rdx, " + n->name + "_len\n";
			text_ += "    syscall\n";
			emit_newline();
		}
        else if(bool_vars_.count(n->name))
        {
            std::string lbl = "__bool" + std::to_string(str_counter_++);
            text_ += "    mov rax, [" + n->name + "]\n";
            text_ += "    test rax, rax\n";
            text_ += "    jz ." + lbl + "_false\n";
            text_ += "    mov rax, 1\n";
            text_ += "    mov rdi, 1\n";
            text_ += "    mov rsi, true_msg\n";
            text_ += "    mov rdx, 4\n";
            text_ += "    syscall\n";
            text_ += "    jmp ." + lbl + "_end\n";
            text_ += "." + lbl + "_false:\n";
            text_ += "    mov rax, 1\n";
            text_ += "    mov rdi, 1\n";
            text_ += "    mov rsi, false_msg\n";
            text_ += "    mov rdx, 5\n";
            text_ += "    syscall\n";
            text_ += "." + lbl + "_end:\n";
            emit_newline();
        }
		else
		{
			text_ += "    mov rax, [" + n->name + "]\n";
			text_ += "    call itoa\n";
			text_ += "    mov rax, 1\n";
			text_ += "    mov rdi, 1\n";
			text_ += "    syscall\n";
			emit_newline();
		}
	}
	else if(auto n = dynamic_cast<NumberLit*>(node->value.get()))
	{
		text_ += "    mov rax, " + std::to_string((int)n->value) + "\n";
		text_ += "    call itoa\n";
		text_ += "    mov rax, 1\n";
		text_ += "    mov rdi, 1\n";
		text_ += "    syscall\n";
		emit_newline();
	}
}

void Codegen::gen_if(IfStmt* node)
{
    std::string end_lbl = "__ifend" + std::to_string(str_counter_++);

    for(size_t i = 0; i < node->branches.size(); i++)
    {
        auto& [cond, body] = node->branches[i];
        std::string next_lbl = "__ifnext" + std::to_string(str_counter_++);
        auto binop = dynamic_cast<BinOp*>(cond.get());

        if(auto id = dynamic_cast<Identifier*>(binop->lhs.get()))
            text_ += "    mov rax, [" + id->name + "]\n";
        else if(auto num = dynamic_cast<NumberLit*>(binop->lhs.get()))
            text_ += "    mov rax, " + std::to_string((int)num->value) + "\n";

        if(auto id = dynamic_cast<Identifier*>(binop->rhs.get()))
            text_ += "    mov rbx, [" + id->name + "]\n";
        else if(auto num = dynamic_cast<NumberLit*>(binop->rhs.get()))
            text_ += "    mov rbx, " + std::to_string((int)num->value) + "\n";

        text_ += "    cmp rax, rbx\n";
        if(binop->op == ">") text_ += "    jle ." + next_lbl + "\n";
        else if(binop->op == "<") text_ += "    jge ." + next_lbl + "\n";
        else if(binop->op == "==") text_ += "    jne ." + next_lbl + "\n";
        else if(binop->op == "!=") text_ += "    je ." + next_lbl + "\n";
        else if(binop->op == ">=") text_ += "    jl ." + next_lbl + "\n";
        else if(binop->op == "<=") text_ += "    jg ." + next_lbl + "\n";

        generate(body.get());
        text_ += "    jmp ." + end_lbl + "\n";
        text_ += "." + next_lbl + ":\n";
    }

    if(node->else_body)
        generate(node->else_body.get());

    text_ += "." + end_lbl + ":\n";
}

void Codegen::gen_repeat(RepeatStmt* node)
{
    std::string lbl = "__repeat" + std::to_string(str_counter_++);

    if(auto num = dynamic_cast<NumberLit*>(node->count.get()))
    {
        text_ += "    mov rcx, " + std::to_string((int)num->value) + "\n";
    }
    else if(auto id = dynamic_cast<Identifier*>(node->count.get()))
    {
        text_ += "    mov rcx, [" + id->name + "]\n";
    }

    text_ += "." + lbl + ":\n";
    text_ += "    push rcx\n";
    generate(node->body.get());
    text_ += "    pop rcx\n";
    text_ += "    dec rcx\n";
    text_ += "    jnz ." + lbl + "\n";
}