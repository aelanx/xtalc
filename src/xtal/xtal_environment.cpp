#include "xtal.h"
#include "xtal_macro.h"

#include "xtal_details.h"
#include "xtal_codebuilder.h"
#include "xtal_filesystem.h"
#include "xtal_lib/xtal_chcode.h"

namespace xtal{

Environment* last_environment_ = 0;
uint_t member_mutate_count_ = 0;
uint_t is_mutate_count_ = 0;
uint_t cache_enable_ = 1;

void enable_cashe(uint_t v){
	cache_enable_ = v;
	member_mutate_count_++;
	is_mutate_count_++;
}

XTAL_TLS_PTR(Environment) environment_;
XTAL_TLS_PTR(VMachine) vmachine_;

namespace{
	ThreadLib empty_thread_lib;
	StdStreamLib empty_std_stream_lib;
	FilesystemLib empty_filesystem_lib;
	AllocatorLib cstd_allocator_lib;
	UTF8ChCodeLib utf8_ch_code_lib;
}

void* AllocatorLib::malloc_align(std::size_t size, std::size_t alignment){
	if(alignment <= sizeof(void*)){
        alignment = sizeof(void*);
	}

    void* p = malloc(size + alignment);
	if(!p){
        return 0;
	}

	char* aligned = (char*)align_p(p, alignment);
	if(aligned==p){
		aligned += alignment;
	}

	*((void**)(aligned-sizeof(void*))) = p;
    return aligned; 
}

void AllocatorLib::free_align(void *mem, std::size_t size, std::size_t alignment){
	if(!mem){
		return;
	}

	char* aligned = (char*)mem;
    void* p = *((void**)(aligned-sizeof(void*)));

    free(p, size+alignment);
}


Environment* environment(){
	return environment_;
}

void set_environment(Environment* environment){
	environment_ = environment;
	last_environment_ = environment;
}

const VMachinePtr& vmachine(){
	return to_smartptr((VMachine*)vmachine_);
}

const VMachinePtr& setup_call(){
	const VMachinePtr& vm = to_smartptr((VMachine*)vmachine_);
	vm->setup_call();
	return vm;
}

const VMachinePtr& setup_call(int_t need_result_count){
	const VMachinePtr& vm = to_smartptr((VMachine*)vmachine_);
	vm->setup_call(need_result_count);
	return vm;
}

VMachine* vmachine2(){
	return vmachine_;
}

VMachinePtr set_vmachine(const VMachinePtr& vm){
	VMachinePtr temp = vmachine_ ? VMachinePtr(vmachine_) : VMachinePtr();

	if(vmachine_){
		vmachine_->dec_ref_count();
	}

	vmachine_ = vm.get();

	if(vmachine_){
		vmachine_->inc_ref_count();
	}

	return temp;
}


////////////////////////////////////

void* xmalloc(size_t size){
	Environment* env = environment_;
		
	if(env->gc_stress_){
		env->object_space_.full_gc();
	}

#if !defined(XTAL_NO_SMALL_ALLOCATOR)
	if(XTAL_SMALL_ALLOCATOR_HANDLE_SIZE(size)){
		return env->so_alloc_.malloc(size);
	}
#endif

	env->used_memory_ += size + 16;

	void* ret = env->setting_.allocator_lib->malloc(size);

	if(!ret){
		env->object_space_.full_gc();
		ret = env->setting_.allocator_lib->malloc(size);

		if(!ret){
			env->object_space_.shrink_to_fit();
			env->setting_.allocator_lib->out_of_memory();
			ret = env->setting_.allocator_lib->malloc(size);

			if(!ret){
				// だめだ。メモリが確保できない。
				// XTAL_MEMORYまでジャンプしよう。

				// XTAL_MEMORYで囲まれていない！もうどうしようもない！
				XTAL_ASSERT(env->set_jmp_buf_);
				
				env->object_space_.print_all_objects();
				env->ignore_memory_assert_= true;
				longjmp(env->jmp_buf_.buf, 1);
			}
		}
	}

	return ret;
} 

void xfree(void* p, size_t size){
	Environment* env = environment_;

	if(!p){
		return;
	}

#if !defined(XTAL_NO_SMALL_ALLOCATOR)
	if(XTAL_SMALL_ALLOCATOR_HANDLE_SIZE(size)){	
		env->so_alloc_.free(p, size);
		return;
	}
#endif

	env->used_memory_ -= size + 16;

	env->setting_.allocator_lib->free(p, size);
}

void* xmalloc_align(size_t size, size_t alignment){
	if(alignment<=8){
		return xmalloc(size);
	}

	Environment* env = environment_;
	
	if(env->gc_stress_){
		env->object_space_.full_gc();
	}

	env->used_memory_ += size + 16;

	void* ret = env->setting_.allocator_lib->malloc_align(size, alignment);

	if(!ret){
		env->object_space_.full_gc();
		ret = env->setting_.allocator_lib->malloc_align(size, alignment);
		if(!ret){
			env->object_space_.shrink_to_fit();
			env->setting_.allocator_lib->out_of_memory();
			ret = env->setting_.allocator_lib->malloc_align(size, alignment);

			if(!ret){
				// だめだ。メモリが確保できない。
				// XTAL_MEMORYまでジャンプしよう。

				// XTAL_MEMORYで囲まれていない！もうどうしようもない！
				XTAL_ASSERT(env->set_jmp_buf_);
				
				env->ignore_memory_assert_= true;
				longjmp(env->jmp_buf_.buf, 1);
			}
		}
	}

	return ret;

}

void xfree_align(void* p, size_t size, size_t alignment){
	if(alignment<=8){
		return xfree(p, size);
	}

	Environment* env = environment_;
	
	if(!p){
		return;
	}

	env->used_memory_ -= size + 16;
	env->setting_.allocator_lib->free_align(p, size, alignment);
}

JmpBuf& protect(){
	// XTAL_PROTECTが入れ子になっている場合assertに引っかかる
	XTAL_ASSERT(!environment_->set_jmp_buf_);

	environment_->set_jmp_buf_ = true;
	return environment_->jmp_buf_;
}

void reset_protect(){
	if(environment_){
		environment_->set_jmp_buf_ = false;
	}
}

bool ignore_memory_assert(){
	return environment_->ignore_memory_assert_;
}

Setting::Setting(){
	thread_lib = &empty_thread_lib;
	std_stream_lib = &empty_std_stream_lib;
	filesystem_lib = &empty_filesystem_lib;
	allocator_lib = &cstd_allocator_lib;
	ch_code_lib = &utf8_ch_code_lib;
}


void initialize (const Setting& setting)
{
	last_environment_ = environment_ = (Environment*)setting.allocator_lib->malloc(sizeof(Environment));
	new(environment_) Environment();
	environment_->initialize(setting);
}

void uninitialize()
{
	if(!environment_){
		return;
	}

	AllocatorLib* allocacator_lib = environment_->setting_.allocator_lib;
	environment_->uninitialize();
	environment_->~Environment();
	allocacator_lib->free(environment_, sizeof(Environment));
	environment_ = 0;
}

void bind();

void Environment::initialize(const Setting& setting){
	setting_ = setting;

//////////

	gc_stress_ = false;

	set_jmp_buf_ = false;
	ignore_memory_assert_ = false;
	used_memory_ = sizeof(Environment);
	
	string_space_.initialize();
	object_space_.initialize();

	lib_ = unchecked_ptr_cast<Lib>(cpp_class<Lib>());
	global_ = unchecked_ptr_cast<Global>(cpp_class<Global>());
	builtin_ = cpp_class<Builtin>();
	builtin_->def(Xid(global), global_);
	builtin_->def(Xid(lib), lib_);

	vm_list_ = XNew<Array>();
	text_map_ = XNew<Map>();

	bind();

	thread_space_.initialize(setting_.thread_lib);
	
	cpp_class<StdinStream>()->inherit(cpp_class<Stream>());
	cpp_class<StdoutStream>()->inherit(cpp_class<Stream>());
	cpp_class<StderrStream>()->inherit(cpp_class<Stream>());

	stdin_ = XNew<StdinStream>();
	stdout_ = XNew<StdoutStream>();
	stderr_ = XNew<StderrStream>();

	builtin_->def(XTAL_DEFINED_ID(stdin), stdin_);
	builtin_->def(XTAL_DEFINED_ID(stdout), stdout_);
	builtin_->def(XTAL_DEFINED_ID(stderr), stderr_);

	enable_gc();

	full_gc();
}

void Environment::uninitialize(){
	thread_space_.join_all_threads();

	object_space_.halt_fibers(); // ファイバーを終わらせる

	clear_cache();
	full_gc();

	builtin_ = null;
	lib_ = null;
	global_ = null;
	vm_list_ = null;

	stdin_ = null;
	stdout_ = null;
	stderr_ = null;
	text_map_ = null;

	string_space_.uninitialize();
	thread_space_.uninitialize();
	object_space_.uninitialize();

#ifndef XTAL_NO_SMALL_ALLOCATOR
	so_alloc_.release();
#endif
}
	
VMachinePtr vmachine_take_over(){
	Environment* environment = environment_;
	if(environment->vm_list_->empty()){
		return xnew<VMachine>();
	}
	VMachinePtr vm = unchecked_ptr_cast<VMachine>(environment->vm_list_->back());
	environment->vm_list_->pop_back();
	return vm;
}

void vmachine_take_back(const VMachinePtr& vm){
	Environment* environment = environment_;
	vm->reset();
	if(environment->vm_list_ && environment->vm_list_->length()<8){
		environment->vm_list_->push_back(vm);
	}
}

void gc(){
	return environment_->object_space_.gc();
}

void full_gc(){
	clear_cache();
	environment_->object_space_.full_gc();

#ifdef XTAL_DEBUG_PRINT
	//printf("used_memory %gKB\n", environment_->used_memory_/1024.0f);
	//environment_->object_space_.print_alive_objects();
	//environment_->so_alloc_.print();
#endif

}

void disable_gc(){
	return environment_->object_space_.disable_gc();
}

void enable_gc(){
	return environment_->object_space_.enable_gc();
}

void set_gc_stress(bool b){
	environment_->gc_stress_ = b;
}

void register_gc(RefCountingBase* p){
	return environment_->object_space_.register_gc(p);
}

uint_t alive_object_count(){
	return environment_->object_space_.alive_object_count();
}

AnyPtr alive_object(uint_t i){
	if(RefCountingBase* p = environment_->object_space_.alive_object(i)){
		return p;
	}
	else{
		return null;
	}
}

const ClassPtr& cpp_class(CppClassSymbolData* key){
	return environment_->object_space_.cpp_class(key);
}

const ClassPtr& cpp_class_index(uint_t index){
	return environment_->object_space_.cpp_class_index(index);
}

const AnyPtr& cpp_value(CppValueSymbolData* key){
	return environment_->object_space_.cpp_value(key);
}

void set_cpp_class(CppClassSymbolData* key, const ClassPtr& cls){
	return environment_->object_space_.set_cpp_class(key, cls);
}

void clear_cache(){
	invalidate_cache_member();
	invalidate_cache_is();
	environment_->member_cache_table_.clear();
	environment_->member_cache_table2_.clear();
	environment_->is_cache_table_.clear();
}

void invalidate_cache_member(){
	member_mutate_count_++;
}

void invalidate_cache_is(){
	member_mutate_count_++;
	is_mutate_count_++;
}


const ClassPtr& builtin(){
	return environment_->builtin_;
}

const LibPtr& lib(){
	return environment_->lib_;
}

const ClassPtr& global(){
	return environment_->global_;
}

namespace{
	
IDPtr intern(const char* str, uint_t size, uint_t hashcode, bool literal){
	if(size<SMALL_STRING_MAX){
		return ID(str, size, ID::small_intern_t());
	}
	else{
		const char* newstr = environment_->string_space_.register_string(str, size, hashcode, literal);
		return ID(newstr, size, ID::intern_t());
	}
}

}

IDPtr intern(const char* str){
	uint_t hashcode, size;
	string_data_size_and_hashcode(str, size, hashcode);
	return intern(str, size, hashcode, false);
}

IDPtr intern(const char* str, uint_t data_size){
	return intern(str, data_size, string_hashcode(str, data_size), false);
}

IDPtr intern(const LongLivedString& str){
	uint_t hashcode, size;
	string_data_size_and_hashcode(str.str(), size, hashcode);
	return intern(str.str(), size, hashcode, true);
}

IDPtr intern(const char* str, uint_t data_size, String::long_lived_t){
	return intern(str, data_size, string_hashcode(str, data_size), true);
}

IDPtr intern(const StringPtr& name){
	const char* str = name->c_str();
	uint_t size = name->data_size();
	uint_t hashcode = string_hashcode(str, size);
	return intern(str, size, hashcode, false);
}

IDPtr fetch_defined_id(uint_t index){
	const StringSpace::Node* node = environment_->string_space_.fetch(index);

	if(node->size<SMALL_STRING_MAX){
		return ID(node->data(), node->size, ID::small_intern_t());
	}
	else{
		return ID(node->data(), node->size, ID::intern_t());
	}
}


void yield_thread(){
	return environment_->thread_space_.yield_thread();
}

void sleep_thread(float_t sec){
	return environment_->thread_space_.sleep_thread(sec);
}

void xlock(){
	environment_->thread_space_.xlock();
}

void xunlock(){
	environment_->thread_space_.xunlock();
}

bool register_thread(Environment* env){
	if(vmachine_){ // 既に登録済みのスレッドであるなら戻る
		return false;
	}
	env->thread_space_.register_thread(env);
	return true;
}

void unregister_thread(Environment* env){
	env->thread_space_.unregister_thread(env);
}

bool register_thread(){
	if(vmachine_){ // 既に登録済みのスレッドであるなら戻る
		return false;
	}

	last_environment_->thread_space_.register_thread(last_environment_);
	return true;
}

void unregister_thread(){
	last_environment_->thread_space_.unregister_thread(last_environment_);
}


ThreadLib* thread_lib(){
	return environment_->setting_.thread_lib;
}

StdStreamLib* std_stream_lib(){
	return environment_->setting_.std_stream_lib;
}

FilesystemLib* filesystem_lib(){
	return environment_->setting_.filesystem_lib;
}

const StreamPtr& stdin_stream(){
	return environment_->stdin_;
}

const StreamPtr& stdout_stream(){
	return environment_->stdout_;
}

const StreamPtr& stderr_stream(){
	return environment_->stderr_;
}

const MapPtr& text_map(){
	return environment_->text_map_;
}

int_t ch_len(char lead){
	return environment_->setting_.ch_code_lib->ch_len(lead);
}

int_t ch_len2(const char* str){
	return environment_->setting_.ch_code_lib->ch_len2(str);
}

int_t ch_inc(const char* data, int_t data_size, char* dest, int_t dest_size){
	return environment_->setting_.ch_code_lib->ch_inc(data, data_size, dest, dest_size);
}

int_t ch_cmp(const char* a, uint_t asize, const char* b, uint_t bsize){
	return environment_->setting_.ch_code_lib->ch_cmp(a, asize, b, bsize);
}

StreamPtr open(const StringPtr& file_name, const StringPtr& mode){
	SmartPtr<FileStream> ret = xnew<FileStream>(file_name, mode);
	if(ret->is_open()){
		return ret;
	}
	set_runtime_error(Xt1("XRE1032", name, file_name));
	return nul<Stream>();
}


struct GCer{
	GCer(int){}
	~GCer(){ full_gc(); }
};

namespace{

	xpeg::ExecutorPtr create_xpeg(const AnyPtr& source, const StringPtr& file_name = empty_string){
		if(const StreamPtr& stream = ptr_cast<Stream>(source)){
			stream->skip_bom();
			return xnew<xpeg::StreamExecutor>(stream, file_name);
		}
		else if(const StringPtr& string = ptr_cast<String>(source)){
			return xnew<xpeg::StreamExecutor>(XNew<StringStream>(string), file_name);
		}
		return nul<xpeg::Executor>();
	}
	
	CodePtr compile_detail(const AnyPtr& source, const StringPtr& file_name){
#ifndef XTAL_NO_PARSER
		GCer gc(0);
		CodeBuilder cb;
		return cb.compile(create_xpeg(source, file_name), file_name);
#else
		return nul<Code>();
#endif
	}

	CodePtr eval_compile_detail(const AnyPtr& source){
#ifndef XTAL_NO_PARSER
		CodeBuilder cb;
		return cb.eval_compile(create_xpeg(source));
#else
		return nul<Code>();
#endif
	}

	CodePtr compile_or_deserialize(const AnyPtr& source, const StringPtr& file_name){
		StreamPtr stream = ptr_cast<Stream>(source);
		if(!stream && ptr_cast<String>(source)){
			stream = XNew<StringStream>(unchecked_ptr_cast<String>(source));
		}

		if(stream){
			u8 head[4] = {0};
			stream->read(head, 4);
			stream->seek(0);

			if(head[0]=='x' && head[1]=='t' && head[2]=='a' && head[3]=='l'){
				return ptr_cast<Code>(stream->deserialize());
			}		
		}

		return compile_detail(stream, file_name);
	}
}

CodePtr compile_file(const StringPtr& file_name){
	if(StreamPtr fs = open(file_name, XTAL_STRING("r"))){
		CodePtr ret = compile_or_deserialize(fs, file_name);
		fs->close();
		return ret;
	}
	return nul<Code>();
}

CodePtr compile(const AnyPtr& source, const StringPtr& source_name){
	return compile_or_deserialize(source, source_name);
}

CodePtr eval_compile(const AnyPtr& source){
	return eval_compile_detail(source);
}

AnyPtr load(const StringPtr& file_name){
	if(CodePtr code = compile_file(file_name)){
		return code->call();
	}
	return undefined;
}

struct RequireData : public Base{
	ArrayPtr require_source_hook_list;
};

void append_require_source_hook(const AnyPtr& hook){
	const SmartPtr<RequireData>& r = cpp_value<RequireData>();
	if(!r->require_source_hook_list){
		r->require_source_hook_list = xnew<Array>();
	}
	r->require_source_hook_list->push_front(hook);
}

void remove_require_source_hook(const AnyPtr& hook){
	const SmartPtr<RequireData>& r = cpp_value<RequireData>();
	if(!r->require_source_hook_list){
		return;
	}

	for(uint_t i=0; i<r->require_source_hook_list->size(); ++i){
		if(XTAL_detail_raweq(r->require_source_hook_list->at(i), hook)){
			r->require_source_hook_list->erase(i);
			break;
		}
	}
}

CodePtr require_source(const StringPtr& name){
	const SmartPtr<RequireData>& r = cpp_value<RequireData>();
	if(r->require_source_hook_list){
		Xfor(hook, r->require_source_hook_list){
			if(MapPtr map = ptr_cast<Map>(hook)){
				AnyPtr ret = map;
				Xfor_cast(StringPtr key, name->split("/")){
					if(MapPtr m = ptr_cast<Map>(ret)){
						ret = m->at(key);
					}
				}

				if(CodePtr code = ptr_cast<Code>(ret)){
					return code;
				}

				XTAL_CHECK_EXCEPT(e){
					return nul<Code>();
				}
			}
			else{
				if(CodePtr ret = ptr_cast<Code>(hook->call(name))){
					return ret;
				}

				XTAL_CHECK_EXCEPT(e){
					return nul<Code>();
				}
			}
		}
	}

	StringPtr temp = Xf1("%s.xtalc", 0, name);
	if(StreamPtr fs = open(name, Xid(r))){
		if(CodePtr code = ptr_cast<Code>(fs->deserialize())){
			return code;
		}
	}
	
	XTAL_CATCH_EXCEPT(e){}

	temp = Xf1("%s.xtal", 0, name);
	if(CodePtr ret = compile_file(temp)){
		return ret;
	}

	XTAL_CATCH_EXCEPT(e){
		if(e->is(cpp_class<CompileError>())){
			XTAL_SET_EXCEPT(e);
			return nul<Code>();
		}
	}

	return nul<Code>();
}

AnyPtr require(const StringPtr& name){
	if(CodePtr ret = require_source(name)){
		return ret->call();
	}
	return undefined;
}

CodePtr source(const char* src, int_t size){
	return compile_detail(xnew<PointerStream>(src, size*sizeof(char)), empty_string);
}

void exec_source(const char* src, int_t size){
	if(CodePtr code = source(src, size)){
		code->call();
	}
	else{
		XTAL_CATCH_EXCEPT(e){
			stderr_stream()->println(e);
		}
	}
}

CodePtr compiled_source(const void* src, int_t size){
	StreamPtr ms = xnew<CompressDecoder>(xnew<PointerStream>(src, size));
	return ptr_cast<Code>(ms->deserialize());
}

void exec_compiled_source(const void* src, int_t size){
	if(CodePtr code = compiled_source(src, size)){
		code->call();
	}
	else{
		XTAL_CATCH_EXCEPT(e){
			stderr_stream()->println(e);
		}
	}
}

}
