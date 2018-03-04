#include "xtal.h"
#include "xtal_macro.h"

namespace xtal{

#ifdef XTAL_DEBUG

int_t inst_inspect_address16(int value, const inst_t* inst, const CodePtr& code){
	int_t pc = (inst_t*)inst - code->bytecode_data();
	return pc + value;
}

StringPtr make_inst_string(const LongLivedString& InstName){
	StringPtr temp = format(XTAL_STRING("%s:"))->call(InstName)->to_s();
	return format(temp)->call()->to_s();
}

StringPtr make_inst_string(const LongLivedString& InstName, 
						const LongLivedString& MemberName1, int_t MemberValue1){
	StringPtr temp = format(XTAL_STRING("%s: %s=%%s"))->call(InstName, MemberName1)->to_s();
	return format(temp)->call(MemberValue1)->to_s();
}

StringPtr make_inst_string(const LongLivedString& InstName, 
						const LongLivedString& MemberName1, int_t MemberValue1,
						const LongLivedString& MemberName2, int_t MemberValue2){
	StringPtr temp = format(XTAL_STRING("%s: %s=%%s, %s=%%s"))->call(InstName, MemberName1, MemberName2)->to_s();
	return format(temp)->call(MemberValue1, MemberValue2)->to_s();
}

StringPtr make_inst_string(const LongLivedString& InstName, 
						const LongLivedString& MemberName1, int_t MemberValue1,
						const LongLivedString& MemberName2, int_t MemberValue2,
						const LongLivedString& MemberName3, int_t MemberValue3){
	StringPtr temp = format(XTAL_STRING("%s: %s=%%s, %s=%%s, %s=%%s"))->call(InstName, MemberName1, MemberName2, MemberName3)->to_s();
	return format(temp)->call(MemberValue1, MemberValue2, MemberValue3)->to_s();
}

StringPtr make_inst_string(const LongLivedString& InstName, 
						const LongLivedString& MemberName1, int_t MemberValue1,
						const LongLivedString& MemberName2, int_t MemberValue2,
						const LongLivedString& MemberName3, int_t MemberValue3,
						const LongLivedString& MemberName4, int_t MemberValue4){
	StringPtr temp = format(XTAL_STRING("%s: %s=%%s, %s=%%s, %s=%%s, %s=%%s"))->call(InstName, MemberName1, MemberName2, MemberName3, MemberName4)->to_s();
	return format(temp)->call(MemberValue1, MemberValue2, MemberValue3, MemberValue4)->to_s();
}

StringPtr make_inst_string(const LongLivedString& InstName, 
						const LongLivedString& MemberName1, int_t MemberValue1,
						const LongLivedString& MemberName2, int_t MemberValue2,
						const LongLivedString& MemberName3, int_t MemberValue3,
						const LongLivedString& MemberName4, int_t MemberValue4,
						const LongLivedString& MemberName5, int_t MemberValue5){					
	StringPtr temp = format(XTAL_STRING("%s: %s=%%s, %s=%%s, %s=%%s, %s=%%s, %s=%%s"))->call(InstName, MemberName1, MemberName2, MemberName3, MemberName4, MemberName5)->to_s();
	return format(temp)->call(MemberValue1, MemberValue2, MemberValue3, MemberValue4, MemberValue5)->to_s();
}

#endif

#ifdef XTAL_DEBUG

StringPtr registerName(i8 r)
{
	return Xf("%s%d")->call(r<0?"n":"r", r<0?-r:r)->to_s();
}

const char *val2Str(u8 val)
{
	switch (val)
	{
	case 0:
		return "null";
	case 1:
		return "undefined";
	case 2:
		return "false";
	case 3:
		return "true";
	}

	return "INVALID_VALUE";
}

int getScopeIdForPc(u32 pc, int depth, const CodePtr& code)
{
	int len = code->scope_info_table_.capacity();
	auto l = new int[len];

	int j = 0;
	for (int i = 0; i < len; ++i)
	{
		if (code->scope_info_table_.at(i).pc > pc)
			break;

		l[j++] = i;
	}

	int id = l[j - depth];

	delete[] l;

	return id;
}

StringPtr valueName(const CodePtr& code, int pc, int num)
{
	//// this isn't a tenable option until getScopeIdForPc works correctly.
	//// which it definitely doesn't.
	//if (num < 0)
	//{
	//	auto scope = code->scope_info_table_.at(getScopeIdForPc(pc, 1, code));
	//	auto id = scope.variable_identifier_offset + scope.variable_size + num;
	//	return code->identifier_table_.at(id).to_s();
	//}

	return Xf("r%d")->call(num)->to_s();
}

const char *classKindStrzzz(u8 kind)
{
	switch (kind)
	{
	case KIND_BLOCK: return "block";
	case KIND_CLASS: return "class";
	case KIND_SINGLETON: return "singleton";
	case KIND_FUN: return "fun";
	case KIND_LAMBDA: return "lambda";
	case KIND_METHOD: return "method";
	case KIND_FIBER: return "fiber";
	}

	return "UnkClassType";
}

StringPtr inspect_range(const CodePtr& code, const inst_t* start, const inst_t* end){

	int sz = 0;
	const inst_t* pc = start;
	StringPtr temp;
	MemoryStreamPtr ms = xnew<MemoryStream>();
	int maxScopeId = 0;
	PODStack<int> scopeStack = PODStack<int>();
	scopeStack.reserve(code->scope_info_table_.capacity());
	scopeStack.push(0);
	scopeStack.push(1);

	// idk if classes can be nested, but whatever
	PODStack<int> classIdStack = PODStack<int>();
	classIdStack.reserve(code->class_info_table_.capacity());

	for(; pc < end;){
		int ipc = (int)(pc - start);
		switch(XTAL_opc(pc)){
		XTAL_NODEFAULT;

#define XTAL_INST_CASE(x) XTAL_CASE(x::NUMBER){ temp = x::inspect(pc, code); sz = x::ISIZE; }

//{INST_INSPECT{{
		XTAL_CASE(InstLine::NUMBER)
		{
			temp = StringPtr("");
			sz = InstLine::ISIZE;
		}
		XTAL_CASE(InstLoadValue::NUMBER)
		{
			i8 res = InstLoadValue::result(pc);
			u8 val = InstLoadValue::value(pc);
			temp = Xf("%s = %s")->call(valueName(code, ipc, res), val2Str(val))->to_s();
			sz = InstLoadValue::ISIZE;
		}
		XTAL_CASE(InstLoadConstant::NUMBER)
		{
			i8 res = InstLoadConstant::result(pc);
			u16 value_number = InstLoadConstant::value_number(pc);
			temp = Xf("%s = \"%s\"")->call(
				valueName(code, ipc, res),
				code->value_table_.at(value_number)
			)->to_s();
			sz = InstLoadConstant::ISIZE;
		}
		XTAL_CASE(InstLoadInt1Byte::NUMBER)
		{
			i8 res = InstLoadInt1Byte::result(pc);
			u8 val = InstLoadInt1Byte::value(pc);
			temp = Xf("%s = %d")->call(valueName(code, ipc, res), val)->to_s();
			sz = InstLoadInt1Byte::ISIZE;
		}
		XTAL_CASE(InstLoadFloat1Byte::NUMBER)
		{
			i8 res = InstLoadFloat1Byte::result(pc);
			u8 val = InstLoadFloat1Byte::value(pc);
			temp = Xf("%s = %.01f")->call(valueName(code, ipc, res), val)->to_s();
			sz = InstLoadFloat1Byte::ISIZE;
		}
		XTAL_INST_CASE(InstLoadCallee);
		XTAL_INST_CASE(InstLoadThis);

		XTAL_CASE(InstCopy::NUMBER)
		{
			int res = InstCopy::result(pc);
			int target = InstCopy::target(pc);
			temp = Xf("%s = %s")->call(
				valueName(code, ipc, res),
				valueName(code, ipc, target)
			)->to_s();
			sz = InstCopy::ISIZE;
		}

		XTAL_INST_CASE(InstInc);
		XTAL_INST_CASE(InstDec);
		XTAL_INST_CASE(InstPos);
		XTAL_INST_CASE(InstNeg);
		XTAL_INST_CASE(InstCom);
		XTAL_CASE(InstAdd::NUMBER)
		{
			int res = InstAdd::result(pc);
			int lhs = InstAdd::lhs(pc);
			int rhs = InstAdd::rhs(pc);
			int assign = InstAdd::assign(pc);

			if (assign)
				temp = Xf("%s += %s")->call(valueName(code, ipc, lhs), valueName(code, ipc, rhs))->to_s();
			else
				temp = Xf("%s = %s + %s")->call(valueName(code, ipc, res), valueName(code, ipc, lhs), valueName(code, ipc, rhs))->to_s();

			sz = InstAdd::ISIZE;
		}
		XTAL_INST_CASE(InstSub);
		XTAL_INST_CASE(InstCat);
		XTAL_INST_CASE(InstMul);
		XTAL_INST_CASE(InstDiv);
		XTAL_INST_CASE(InstMod);
		XTAL_INST_CASE(InstAnd);
		XTAL_INST_CASE(InstOr);
		XTAL_INST_CASE(InstXor);
		XTAL_INST_CASE(InstShl);
		XTAL_INST_CASE(InstShr);
		XTAL_INST_CASE(InstUshr);
		XTAL_INST_CASE(InstAt);
		XTAL_INST_CASE(InstSetAt);
		XTAL_INST_CASE(InstGoto);
		XTAL_INST_CASE(InstNot);
		XTAL_INST_CASE(InstIf);
		XTAL_INST_CASE(InstIfEq);
		XTAL_INST_CASE(InstIfLt);
		XTAL_INST_CASE(InstIfRawEq);
		XTAL_INST_CASE(InstIfIs);
		XTAL_INST_CASE(InstIfIn);
		XTAL_INST_CASE(InstIfUndefined);
		XTAL_INST_CASE(InstIfDebug);
		XTAL_INST_CASE(InstPush);
		XTAL_INST_CASE(InstPop);
		XTAL_INST_CASE(InstAdjustValues);

		//XTAL_INST_CASE(InstLocalVariable);
		XTAL_INST_CASE(InstSetLocalVariable);

		XTAL_CASE(InstLocalVariable::NUMBER)
		{
			int res = InstLocalVariable::result(pc);
			int number = InstLocalVariable::number(pc);
			int depth = InstLocalVariable::depth(pc);

			int scope = getScopeIdForPc(ipc, depth, code);
			int id = code->scope_info_table_.at(scope).variable_identifier_offset + number;
/*
			temp = Xf("%s = %s")->call(
				valueName(code, ipc, res),
				code->identifier_table_.at(id)
			)->to_s();
*/
			sz = InstLocalVariable::ISIZE;
		}

		//XTAL_CASE(InstSetLocalVariable::NUMBER)
		//{
		//	int value = InstSetLocalVariable::value(pc);
		//	int value_number = InstSetLocalVariable::value_number(pc);
		//	int id = code->scope_info_table_.at(1).variable_identifier_offset + value_number;
		//	temp = Xf("%s = %s")->call(
		//		code->identifier_table_.at(id),
		//		valueName(code, ipc, value)
		//	)->to_s();
		//	sz = InstSetLocalVariable::ISIZE;
		//}

		XTAL_CASE(InstInstanceVariable::NUMBER)
		{
			i8 res = InstInstanceVariable::result(pc);
			u16 info_number = InstInstanceVariable::info_number(pc);
			u8 number = InstInstanceVariable::number(pc);

			u16 classNameId = code->class_info_table_.at(info_number).name_number;
			u16 identifier_number = code->class_info_table_.at(info_number).instance_variable_identifier_offset + number;

			temp = Xf("%s = this.%s")->call(
				registerName(res),
				code->identifier_table_.at(identifier_number)
			)->to_s();
			sz = InstInstanceVariable::ISIZE;
		}

		XTAL_CASE(InstSetInstanceVariable::NUMBER)
		{
			int value = InstSetInstanceVariable::value(pc);
			u16 info_number = InstSetInstanceVariable::info_number(pc);
			u8 number = InstSetInstanceVariable::number(pc);

			u16 classNameId = code->class_info_table_.at(info_number).name_number;
			u16 identifier_number = code->class_info_table_.at(info_number).instance_variable_identifier_offset + number;

			temp = Xf("this.%s = %s")->call(
				//code->identifier_table_.at(classNameId),
				code->identifier_table_.at(identifier_number),
				registerName(value)
			)->to_s();
			sz = InstSetInstanceVariable::ISIZE;
		}

		XTAL_INST_CASE(InstInstanceVariableByName);
		XTAL_INST_CASE(InstSetInstanceVariableByName);
		XTAL_CASE(InstFilelocalVariable::NUMBER)
		{
			int res = InstFilelocalVariable::result(pc);
			int value_number = InstFilelocalVariable::value_number(pc);
			int id = code->scope_info_table_.at(1).variable_identifier_offset + value_number;
			temp = Xf("%s = %s")->call(
				valueName(code, ipc, res),
				code->identifier_table_.at(id)
			)->to_s();
			sz = InstFilelocalVariable::ISIZE;
		}

		XTAL_CASE(InstSetFilelocalVariable::NUMBER)
		{
			int value = InstSetFilelocalVariable::value(pc);
			int value_number = InstSetFilelocalVariable::value_number(pc);
			int id = code->scope_info_table_.at(1).variable_identifier_offset + value_number;
			temp = Xf("%s = %s")->call(
				code->identifier_table_.at(id),
				valueName(code, ipc, value)
			)->to_s();
			sz = InstSetFilelocalVariable::ISIZE;
		}

		XTAL_CASE(InstFilelocalVariableByName::NUMBER)
		{
			i8 res = InstFilelocalVariableByName::result(pc);
			u16 identifier_number = InstFilelocalVariableByName::identifier_number(pc);
			temp = Xf("%s = %s")->call(registerName(res), code->identifier_table_.at(identifier_number))->to_s();
			sz = InstFilelocalVariableByName::ISIZE;
		}

		XTAL_CASE(InstSetFilelocalVariableByName::NUMBER)
		{
			i8 value = InstSetFilelocalVariableByName::value(pc);
			u16 identifier_number = InstSetFilelocalVariableByName::identifier_number(pc);
			temp = Xf("%s = %s")->call(code->identifier_table_.at(identifier_number), registerName(value))->to_s();
			sz = InstSetFilelocalVariableByName::ISIZE;
		}

		XTAL_CASE(InstMember::NUMBER)
		{
			i8 result = InstMember::result(pc);
			i8 target = InstMember::target(pc);
			i16 primary = InstMember::primary(pc);
			temp = Xf("%s = %s::%s")->call(
				registerName(result),
				registerName(target),
				code->identifier_table_.at(primary)
			)->to_s();
			sz = InstMember::ISIZE;
		}
		XTAL_INST_CASE(InstMemberEx);

		XTAL_CASE(InstCall::NUMBER)
		{
			int result = InstCall::result(pc);
			int need_result = InstCall::need_result(pc);
			int target = InstCall::target(pc);
			int stack_base = InstCall::stack_base(pc);
			int numArgs = InstCall::ordered(pc);

			StringPtr argsStr = StringPtr("");

			for (int i = 0; i < numArgs; ++i)
			{
				argsStr = argsStr->cat(Xf("%sr%d")->call(
					(i > 0) ? ", " : "",
					stack_base + i
				)->to_s());
			}
			// 0075(0016):InstCall: result=-3, need_result=1, target=0, stack_base=1, ordered=3

			if (need_result)
			{
				temp = Xf("%s = %s(%s)")->call(
					valueName(code, ipc, result),
					valueName(code, ipc, target),
					argsStr
				)->to_s();
			}
			else
			{
				temp = Xf("%s(%s)")->call(
					valueName(code, ipc, target),
					argsStr
				)->to_s();
			}

			sz = InstCall::ISIZE;
		}

		XTAL_INST_CASE(InstCallEx);

		XTAL_CASE(InstSend::NUMBER)
		{
			int result = InstSend::result(pc);
			int need_result = InstSend::need_result(pc);
			int target = InstSend::target(pc);
			int primary = InstSend::primary(pc);
			int secondary = InstSend::secondary(pc);

			if (need_result)
			{
				temp = Xf("%s = %s.%s()")->call(
					valueName(code, ipc, result),
					valueName(code, ipc, target),
					code->identifier_table_.at(primary)
				)->to_s();
			}
			else
			{
				temp = Xf("%s.%s()")->call(
					valueName(code, ipc, target),
					code->identifier_table_.at(primary)
				)->to_s();
			}

			sz = InstSend::ISIZE;
		}

		XTAL_INST_CASE(InstSendEx);

		XTAL_CASE(InstProperty::NUMBER)
		{
			i8 result = InstProperty::result(pc);
			i8 target = InstProperty::target(pc);
			i16 primary = InstProperty::primary(pc);
			temp = Xf("%s = %s.%s")->call(
				registerName(result),
				registerName(target),
				code->identifier_table_.at(primary)
			)->to_s();
			sz = InstMember::ISIZE;
		}

		XTAL_CASE(InstSetProperty::NUMBER)
		{
			i8 target = InstSetProperty::target(pc);
			i16 primary = InstSetProperty::primary(pc);
			int stack_base = InstSetProperty::stack_base(pc);
			temp = Xf("%s.%s(<stack_base=%s>)")->call(
				registerName(target),
				code->identifier_table_.at(primary),
				registerName(stack_base)
			)->to_s();
			sz = InstSetProperty::ISIZE;
		}

		XTAL_CASE(InstScopeBegin::NUMBER)
		{
			temp = InstScopeBegin::inspect(pc, code);
			sz = InstScopeBegin::ISIZE;
			int num = InstScopeBegin::info_number(pc);
			scopeStack.push(num);
		}
		XTAL_CASE(InstScopeEnd::NUMBER)
		{
			temp = InstScopeEnd::inspect(pc, code);
			sz = InstScopeEnd::ISIZE;
			//scopeStack.pop();
		}

		XTAL_CASE(InstReturn::NUMBER)
		{
			int base = InstReturn::base(pc);
			int result_count = InstReturn::result_count(pc);

			temp = StringPtr("return ");

			for (int i = 0; i < result_count; ++i)
			{
				temp = temp->cat(Xf("%sr%d")->call(
					(i > 0) ? ", " : "",
					base + i
				)->to_s());
			}

			sz = InstReturn::ISIZE;
		}

		XTAL_INST_CASE(InstYield);
		XTAL_INST_CASE(InstExit);
		XTAL_INST_CASE(InstRange);
		XTAL_INST_CASE(InstOnce);
		XTAL_INST_CASE(InstSetOnce);

		XTAL_CASE(InstMakeArray::NUMBER)
		{
			int res = InstMakeArray::result(pc);

			temp = Xf("%s = []")->call(
				registerName(res)
			)->to_s();
			sz = InstMakeArray::ISIZE;
		}
		XTAL_INST_CASE(InstArrayAppend);

		XTAL_CASE(InstMakeMap::NUMBER)
		{
			int res = InstMakeMap::result(pc);

			temp = Xf("%s = {}")->call(
				registerName(res)
			)->to_s();
			sz = InstMakeMap::ISIZE;
		}
		
		XTAL_CASE(InstMapInsert::NUMBER)
		{
			int target = InstMapInsert::target(pc);
			int key = InstMapInsert::key(pc);
			int value = InstMapInsert::value(pc);

			temp = Xf("%s[%s] = %s")->call(
				registerName(target),
				registerName(key),
				registerName(value)
			)->to_s();
			sz = InstMapInsert::ISIZE;
		}

		XTAL_INST_CASE(InstMapSetDefault);

		XTAL_CASE(InstClassBegin::NUMBER)
		{
			sz = InstClassBegin::ISIZE;
			int info_number = InstClassBegin::info_number(pc);

			u16 classNameId = code->class_info_table_.at(info_number).name_number;
			temp = Xf("%s %s")->call(
				classKindStrzzz(code->class_info_table_.at(info_number).kind),
				code->identifier_table_.at(classNameId)
			)->to_s();

			scopeStack.push(++maxScopeId);
			classIdStack.push(info_number);
		}
		XTAL_CASE(InstClassEnd::NUMBER)
		{
			temp = InstClassEnd::inspect(pc, code);
			sz = InstClassEnd::ISIZE;
			classIdStack.pop();
			scopeStack.pop();
		}

		XTAL_CASE(InstDefineClassMember::NUMBER)
		{
			int classId = classIdStack.top();
			int number = InstDefineClassMember::number(pc);
			int primary = InstDefineClassMember::primary(pc);
			int value = InstDefineClassMember::value(pc);
			u16 classNameId = code->class_info_table_.at(classId).name_number;
			u16 nameNum = code->class_info_table_.at(classId).instance_variable_identifier_offset + number;

			temp = Xf("%s::%s = %s")->call(
				code->identifier_table_.at(classNameId),
				code->identifier_table_.at(nameNum),
				registerName(value)
			)->to_s();
			sz = InstDefineClassMember::ISIZE;
		}
		XTAL_INST_CASE(InstDefineMember);

		XTAL_CASE(InstMakeFun::NUMBER)
		{
			int result = InstMakeFun::result(pc);
			int info_number = InstMakeFun::info_number(pc);
			int nextAddr = ipc + InstMakeFun::address(pc);

			auto info = code->xfun_info_table_.at(info_number);
			StringPtr argStr = StringPtr("");

			// 0057(0012):InstMakeFun: result=0, info_number=2, address=130

			for (int i = 0; i < info.min_param_count; ++i)
			{
				argStr = argStr->cat(Xf("%s%s")->call(
					(i > 0) ? ", " : "",
					code->identifier_table_.at(info.variable_identifier_offset + i)
				)->to_s());
			}

			temp = Xf("%s %s(%s) // next_addr=%04d").call(
				classKindStrzzz(info.kind),
				code->identifier_table_.at(info.name_number),
				argStr,
				nextAddr
			).to_s();

			sz = InstMakeFun::ISIZE;
		}
		XTAL_INST_CASE(InstMakeInstanceVariableAccessor);
		XTAL_INST_CASE(InstTryBegin);
		XTAL_INST_CASE(InstTryEnd);
		XTAL_INST_CASE(InstPushGoto);
		XTAL_INST_CASE(InstPopGoto);
		XTAL_INST_CASE(InstThrow);
		XTAL_INST_CASE(InstAssert);
		XTAL_INST_CASE(InstBreakPoint);
		XTAL_INST_CASE(InstMAX);
//}}INST_INSPECT}
	} ms->put_s(Xf("%04d(%04d):%s\n")->call((int_t)(pc-start), code->compliant_lineno(pc), temp)->to_s()); pc += sz; }

	ms->seek(0);
	return ms->get_s(ms->size());
}

#else

StringPtr inspect_range(const CodePtr&, const inst_t*, const inst_t*){
	return XTAL_STRING("");
}

#endif

int_t inst_size(uint_t no){
	static u8 sizelist[] = {
//{INST_SIZE{{
	InstLine::ISIZE,
	InstLoadValue::ISIZE,
	InstLoadConstant::ISIZE,
	InstLoadInt1Byte::ISIZE,
	InstLoadFloat1Byte::ISIZE,
	InstLoadCallee::ISIZE,
	InstLoadThis::ISIZE,
	InstCopy::ISIZE,
	InstInc::ISIZE,
	InstDec::ISIZE,
	InstPos::ISIZE,
	InstNeg::ISIZE,
	InstCom::ISIZE,
	InstAdd::ISIZE,
	InstSub::ISIZE,
	InstCat::ISIZE,
	InstMul::ISIZE,
	InstDiv::ISIZE,
	InstMod::ISIZE,
	InstAnd::ISIZE,
	InstOr::ISIZE,
	InstXor::ISIZE,
	InstShl::ISIZE,
	InstShr::ISIZE,
	InstUshr::ISIZE,
	InstAt::ISIZE,
	InstSetAt::ISIZE,
	InstGoto::ISIZE,
	InstNot::ISIZE,
	InstIf::ISIZE,
	InstIfEq::ISIZE,
	InstIfLt::ISIZE,
	InstIfRawEq::ISIZE,
	InstIfIs::ISIZE,
	InstIfIn::ISIZE,
	InstIfUndefined::ISIZE,
	InstIfDebug::ISIZE,
	InstPush::ISIZE,
	InstPop::ISIZE,
	InstAdjustValues::ISIZE,
	InstLocalVariable::ISIZE,
	InstSetLocalVariable::ISIZE,
	InstInstanceVariable::ISIZE,
	InstSetInstanceVariable::ISIZE,
	InstInstanceVariableByName::ISIZE,
	InstSetInstanceVariableByName::ISIZE,
	InstFilelocalVariable::ISIZE,
	InstSetFilelocalVariable::ISIZE,
	InstFilelocalVariableByName::ISIZE,
	InstSetFilelocalVariableByName::ISIZE,
	InstMember::ISIZE,
	InstMemberEx::ISIZE,
	InstCall::ISIZE,
	InstCallEx::ISIZE,
	InstSend::ISIZE,
	InstSendEx::ISIZE,
	InstProperty::ISIZE,
	InstSetProperty::ISIZE,
	InstScopeBegin::ISIZE,
	InstScopeEnd::ISIZE,
	InstReturn::ISIZE,
	InstYield::ISIZE,
	InstExit::ISIZE,
	InstRange::ISIZE,
	InstOnce::ISIZE,
	InstSetOnce::ISIZE,
	InstMakeArray::ISIZE,
	InstArrayAppend::ISIZE,
	InstMakeMap::ISIZE,
	InstMapInsert::ISIZE,
	InstMapSetDefault::ISIZE,
	InstClassBegin::ISIZE,
	InstClassEnd::ISIZE,
	InstDefineClassMember::ISIZE,
	InstDefineMember::ISIZE,
	InstMakeFun::ISIZE,
	InstMakeInstanceVariableAccessor::ISIZE,
	InstTryBegin::ISIZE,
	InstTryEnd::ISIZE,
	InstPushGoto::ISIZE,
	InstPopGoto::ISIZE,
	InstThrow::ISIZE,
	InstAssert::ISIZE,
	InstBreakPoint::ISIZE,
	InstMAX::ISIZE,
//}}INST_SIZE}
	};

	if(no>=InstMAX::NUMBER){
		return 0;
	}

	return sizelist[no];
}

}
