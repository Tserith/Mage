#include "decompiler.hpp"

string evaluate(shared_ptr<expr> expr, shared_ptr<func> func)
{
	string temp;

	switch (expr->type)
	{
	case leaf::none: // edge node
		switch (expr->operation.type)
		{
		case optr::add:
			temp = " + ";
			break;
		case optr::sub:
			temp = " - ";
			break;
		case optr::mul:
			temp = " * ";
			break;
		case optr::div:
			temp = " / ";
			break;
		case optr::or:
			temp = " | ";
			break;
		case optr::xor:
			temp = " ^ ";
			break;
		case optr::and :
			temp = " & ";
			break;
		case optr::not:
			return " ~" + evaluate(expr->left, func);
		case optr::shl:
			temp = " << ";
			break;
		case optr::shr:
			temp = " >> ";
			break;
		case optr::deref:
			return "*" + evaluate(expr->left, func);
		case optr::ref:
			return "&" + evaluate(expr->left, func);
		default:
			printf("Unhandled optr: %x\n", expr->operation.type);
			return "";
		}
		break;
	case leaf::raw:
	{
		stringstream stream;
		stream << hex << expr->value;
		return string(stream.str());
	}
	case leaf::reg:
		return STR_REGISTER[expr->value].data;
	case leaf::var:
		return func->getVar(expr->value, varType::stack)->name;
	}

	return evaluate(expr->left, func) + temp + evaluate(expr->right, func);
}

shared_ptr<expr> reduceTree(shared_ptr<expr> tree, shared_ptr<func> func)
{
	// base case - leaf node reached
	if (tree->type != leaf::none) return tree;

	// ignore old assignments
	if (tree->operation.type == optr::mov)
	{
		return reduceTree(tree->right, func);
	}

	// reduce arithmetic operations
	// if two next nodes have right leaf nodes with values
	if (tree->right && tree->right->type == leaf::raw &&
		tree->left && tree->left->right && tree->left->right->type == leaf::raw)
	{
		if ((tree->operation.type == optr::add || tree->operation.type == optr::sub)
			&& (tree->left->operation.type == optr::add || tree->left->operation.type == optr::sub))
		{
			auto newNode = make_shared<expr>();
			auto num = make_shared<expr>();

			num->type = leaf::raw;

			if ((tree->operation.type == optr::sub) ^ (tree->left->operation.type == optr::sub))
			{
				num->value = tree->left->right->value - tree->right->value;
			}
			else
			{
				num->value = tree->left->right->value + tree->right->value;
			}

			newNode->left = tree->left->left;
			newNode->right = num;
			newNode->operation.type = tree->left->operation.type;
			tree = newNode;
		}
	}

	tree->left = reduceTree(tree->left, func);
	if (tree->right)
		tree->right = reduceTree(tree->right, func);

	// deref and ref cancel out
	if (tree->operation.type == optr::deref && tree->left->operation.type == optr::ref)
	{
		// need to adjust type too
		
		return tree->left->left;
	}

	// if operands are leaf nodes
	if (tree->left->type != leaf::none && (!tree->right || tree->right->type != leaf::none))
	{
		auto newNode = make_shared<expr>();
		newNode->type = leaf::raw;

		switch (tree->left->type)
		{
		case leaf::reg:
			if (tree->operation.type == optr::sub && tree->left->value == ZYDIS_REGISTER_RSP)
			{
				auto ref = make_shared<expr>();
				ref->left = newNode;
				ref->operation.type = optr::ref;

				// create variable if this is first reference
				func->getVar(tree->right->value, varType::stack);

				newNode->type = leaf::var;
				newNode->value = tree->right->value;
				return ref;
			}
			break;
		case leaf::raw:
			switch (tree->operation.type)
			{
			case optr::add:
				newNode->value = tree->left->value + tree->right->value;
				return newNode;
			case optr::sub:
				newNode->value = tree->left->value - tree->right->value;
				return newNode;
			case optr::mul:
				newNode->value = tree->left->value * tree->right->value;
				return newNode;
			case optr::div:
				newNode->value = tree->left->value / tree->right->value;
				return newNode;
			case optr::or:
				newNode->value = tree->left->value | tree->right->value;
				return newNode;
			case optr::xor:
				newNode->value = tree->left->value ^ tree->right->value;
				return newNode;
			case optr::and:
				newNode->value = tree->left->value & tree->right->value;
				return newNode;
			case optr::not:
				newNode->value = ~tree->left->value;
				return newNode;
			case optr::shl:
				newNode->value = tree->left->value << tree->right->value;
				return newNode;
			case optr::shr:
				newNode->value = tree->left->value >> tree->right->value;
				return newNode;
			case optr::deref:
				return tree;
			case optr::ref:
				return tree;
			default:
				printf("Unhandled optr: %x\n", tree->operation.type);
				return tree;
			}
		}
	}

	return tree;
}

string decompile(shared_ptr<func> func)
{
	string ccode = func->name + "()\n{\n";

	// print variables

	// cycle through icode
	for (auto ir = func->code; ir != nullptr; ir = ir->next)
	{
		// handle local variables
		// handle assignments
		// handle code constructs upon branches

		if (ir->expr)
		{
			// root node shouldn't be altered
			if (ir->expr->left)
				ir->expr->left = reduceTree(ir->expr->left, func);
			if (ir->expr->right)
				ir->expr->right = reduceTree(ir->expr->right, func);
		}

		switch (ir->type)
		{
		case itype::assign:
			ccode += "\t" + evaluate(ir->expr->left, func);

			switch (ir->expr->operation.type)
			{
			case optr::mov:
				ccode += " = ";
				break;
			case optr::add:
				ccode += " += ";
				break;
			case optr::sub:
				ccode += " -= ";
				break;
			case optr::mul:
				ccode += " *= ";
				break;
			case optr::div:
				ccode += " /= ";
				break;
			case optr::or:
				ccode += " |= ";
				break;
			case optr::xor:
				ccode += " ^= ";
				break;
			case optr::and:
				ccode += " &= ";
				break;
			case optr::not:
				ccode += " ~= ";
				break;
			}

			if (ir->expr->operation.type != optr::not)
			{
				ccode += evaluate(ir->expr->right, func) + ";\n";
			}
			break;
		case itype::call:
		{
			auto itr = functions.find(ir->expr->value);
			if (itr != functions.end())
			{
				ccode += "\t";
				ccode += itr->second->name + "();\n";
			}
			break;
		}
		case itype::ret:
			ccode += "\n\t";
			ccode += "return;\n";
			break;
		}
	}

	ccode += "}\n";

	return ccode;
}