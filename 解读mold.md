# 解读mold

根据注释和个人的理解，分为如下这么几大部分

1. 解析所有的输入，包含命令行参数，输入的各种文件
2. 对于输入做链接器最基本的处理，包含符号解析，段合并，符号检查之类的
3. 创建一些synthetic的内容，包括一些段和符号
4. 将所有段、符号进行扫描以及按照需求进行排序，添加到全局的ctx中
5. 计算与修正一些具体的信息，固定生成产物的memory layout
6. 修正某些地址，确保固定file layout
7. 将所有文件拷贝到输出文件中
8. 结束的清理操作

# elf_main

elf/main.cc

```c++
ctx.cmdline_args = expand_response_files(ctx, argv);
```

- 将命令行参数放到ctx.cmdline_args :ok:

```c++
std::vector<std::string> file_args = parse_nonpositional_args(ctx);
```

- 解析命令行参数 在cmdline.cc

- parse_nonpositional_args:ok:

  - ```c++
    if (read_arg("o") || read_arg("output")) {
          ctx.arg.output = arg;
    }
    ```

    - output放了可执行文件名

  - ```c++
    else {
          if (args[0][0] == '-')
            Fatal(ctx) << "unknown command line option: " << args[0];
          remaining.push_back(std::string(args[0]));
          args = args.subspan(1);
    }
    ```

    - 把剩下的命令行参数放进remaining,args扩展一次

  - ```c++
    if (ctx.arg.thread_count == 0)
        ctx.arg.thread_count = get_default_thread_count();
    ```

    // 2

  - ```c++
    ctx.arg.undefined.push_back(ctx.arg.entry);
    ```

  - ```c++
    return remaining;
    ```

    - 返回除了可执行文件以外剩余的命令行参数

```c++
// If no -m option is given, deduce it from input files.
if (ctx.arg.emulation.empty())
    ctx.arg.emulation = deduce_machine_type(ctx, file_args); 
```

- 如果命令行没有-m，根据输入的文件进行推断 :ok:

```c++
// Redo if -m is not x86-64.
  if constexpr (is_x86_64<E>)
    if (ctx.arg.emulation != X86_64::target_name)
      return redo_main(ctx, argc, argv);
```

- 如果不是x86_64执行redo_main函数

- redo_main

  - 简单的根据命令行参数指定的target来选择对应的模板类型进行特化

  - ```c++
    case MachineType::ARM32:
        return elf_main<ARM32>(argc, argv);
    ```

```c++
Timer t_all(ctx, "all");
install_signal_handler();
```

```
acquire_global_lock(ctx);
tbb::global_control tbb_cont(tbb::global_control::max_allowed_parallelism,
                               ctx.arg.thread_count);
```

```c++
// Parse input files
read_input_files(ctx, file_args);
```

## read_input_files

- 解析输入文件
- elf/main.cc

- ```c++
   while (!args.empty()) {
      // 取得输入文件名 
      std::string_view arg = args[0];
      else {
        // 读输入文件
        read_file(ctx, MappedFile<Context<E>>::must_open(ctx, std::string(arg)));
      }
   }
  if (ctx.objs.empty())
      Fatal(ctx) << "no input files";
  
    ctx.tg.wait();
  ```

### must_open

- 貌似是个打开文件的操作

- common/common.h

- ```cc
  template <typename Context>
  MappedFile<Context> *
  MappedFile<Context>::must_open(Context &ctx, std::string path) {
    if (MappedFile *mf = MappedFile::open(ctx, path))
      return mf;
    Fatal(ctx) << "cannot open " << path << ": " << errno_string();
  }
  ```

### read_file

- 读文件

- elf/main.cc

- ```c++
  switch (get_file_type(ctx, mf)) {
      case FileType::ELF_OBJ:
          ctx.objs.push_back(new_object_file(ctx, mf, ""));
          return;
  }
  ```

#### new_object_file

- 新建目标文件

- elf/main.cc

- ```c++
  // 猜测是计数输入的目标文件数
  static Counter count("parsed_objs");
  // 数目++
  count++;
  
  // 检查是否架构相同
  check_file_compatibility(ctx, mf);
  
  bool in_lib = ctx.in_lib || (!archive_name.empty() && !ctx.whole_archive);
  ObjectFile<E> *file = ObjectFile<E>::create(ctx, mf, archive_name, in_lib);
  file->priority = ctx.file_priority++;
  ctx.tg.run([file, &ctx] { file->parse(ctx); });
  if (ctx.arg.trace)
      SyncOut(ctx) << "trace: " << *file;
  return file;
  ```

##### create :ok:

- /elf/input-files.cc

- ```c++
  template <typename E>
  ObjectFile<E> *
  ObjectFile<E>::create(Context<E> &ctx, MappedFile<Context<E>> *mf,
                        std::string archive_name, bool is_in_lib) {
    ObjectFile<E> *obj = new ObjectFile<E>(ctx, mf, archive_name, is_in_lib);
    ctx.obj_pool.emplace_back(obj);
    return obj;
  }
  ```

  - 这里创造ObjectFile实际上大部分由InputFile实例创造

    - elf/input-file.cc

    - ```c++
      template <typename E>
      ObjectFile<E>::ObjectFile(Context<E> &ctx, MappedFile<Context<E>> *mf,
                                std::string archive_name, bool is_in_lib)
        : InputFile<E>(ctx, mf), archive_name(archive_name), is_in_lib(is_in_lib) {
        this->is_alive = !is_in_lib;
      }
      ```

      - ```c++
        template <typename E>
        InputFile<E>::InputFile(Context<E> &ctx, MappedFile<Context<E>> *mf)
          : mf(mf), filename(mf->name) {
              if (mf->size < sizeof(ElfEhdr<E>))
                Fatal(ctx) << *this << ": file too small";
              if (memcmp(mf->data, "\177ELF", 4))
                Fatal(ctx) << *this << ": not an ELF file";
        
              //创建ELF header
              ElfEhdr<E> &ehdr = *(ElfEhdr<E> *)mf->data;
              is_dso = (ehdr.e_type == ET_DYN);
        		
              // e_shoff是节头表文件偏移量   所以sh_begin应该是一个ELF header的末尾
              ElfShdr<E> *sh_begin = (ElfShdr<E> *)(mf->data + ehdr.e_shoff);
        
              // e_shnum是节的数量
              // e_shnum contains the total number of sections in an object file.
              // Since it is a 16-bit integer field, it's not large enough to
              // represent >65535 sections. If an object file contains more than 65535
              // sections, the actual number is stored to sh_size field.
              i64 num_sections = (ehdr.e_shnum == 0) ? sh_begin->sh_size : ehdr.e_shnum;
        
              if (mf->data + mf->size < (u8 *)(sh_begin + num_sections))
                Fatal(ctx) << mf->name << ": e_shoff or e_shnum corrupted: "
                           << mf->size << " " << num_sections;
              // elf_sections在sh_begin 与 sh_begin + num_sections之间
              elf_sections = {sh_begin, sh_begin + num_sections};
        
              // e_shstrndx is a 16-bit field. If .shstrtab's section index is
              // too large, the actual number is stored to sh_link field.
              // ehdr.e_shstrndx是节头字符串表索引
              i64 shstrtab_idx = (ehdr.e_shstrndx == SHN_XINDEX)
                ? sh_begin->sh_link : ehdr.e_shstrndx;
              // 获取节头字符串表
              shstrtab = this->get_string(ctx, shstrtab_idx);
        }
        ```

        - 创造InputFile实例
        - class InputFile类中有mf和std::string filename;
        - shstrtab的值是"\000.symtab\000.strtab\000.shstrtab\000.text\000.data\000.bss\000.comment\000.note.GNU-stack\000.ARM.attributes"

- ```c++
  ctx.obj_pool.emplace_back(obj);
  ```

  - tbb

- ```
  ctx.tg.run([file, &ctx] { file->parse(ctx); });
  ```

  - mold以链接速度快出名，而其快的原因之一就是充分利用了多线程，实际进行多线程操作的地方是在这里，ctx.tg.run，tg则是一个tbb::task_group，简而言之就是在这里开启了多线程的解析input file。
  - 做了一些简单的in_lib参数处理，因为archive的链接机制默认是按需链接，而不是像shared file一样全部链接，之后在这里创建了object file并且开始parse。关于创建和parse的细节在后面再说。

##### parse

- elf/input-files.cc

- ```c++
  template <typename E>
  void ObjectFile<E>::parse(Context<E> &ctx) {
      // sections是ObjectFile类中的一个成员
      // 根据段的数目进行对sections内成员数目的重置
      sections.resize(this->elf_sections.size());
  
      //找到符号表段
      symtab_sec = this->find_section(SHT_SYMTAB);
      if (symtab_sec) {
          // In ELF, all local symbols precede global symbols in the symbol table.
          // sh_info has an index of the first global symbol.
          // 第一个global的符号的索引在符号表段的sh_info上
          this->first_global = symtab_sec->sh_info;
          // 输入文件的
          this->elf_syms = this->template get_data<ElfSym<E>>(ctx, *symtab_sec);
          // sh_link就是保存了symbol_strtab的地址
          this->symbol_strtab = this->get_string(ctx, symtab_sec->sh_link);
  
          if (ElfShdr<E> *shdr = this->find_section(SHT_SYMTAB_SHNDX))
            symtab_shndx_sec = this->template get_data<U32<E>>(ctx, *shdr);
      }
      // 初始化段
      initialize_sections(ctx);
      // 初始化符号
      initialize_symbols(ctx);
      sort_relocations(ctx);
      parse_ehframe(ctx);
  }
  ```

###### find_section :ok:

- ```c++
  template <typename E>
  ElfShdr<E> *InputFile<E>::find_section(i64 type) {
    for (ElfShdr<E> &sec : elf_sections)
      if (sec.sh_type == type)
        return &sec;
    return nullptr;
  }
  ```

- ![QQ截图20231209190445](D:\Personal\Desktop\img\QQ截图20231209190445.png)

###### initialize_sections :ok:

- ```c++
  template <typename E>
  void ObjectFile<E>::initialize_sections(Context<E> &ctx) {
      // Read sections
  	for (i64 i = 0; i < this->elf_sections.size(); i++) {
          const ElfShdr<E> &shdr = this->elf_sections[i];
          switch (shdr.sh_type) {
              case SHT_GROUP:
              case SHT_REL:
              case SHT_RELA:
              case SHT_SYMTAB:
              case SHT_SYMTAB_SHNDX:
              case SHT_STRTAB:
              case SHT_NULL:
                  break;
              default:
                  // .text .data .bss .comment .note.GNU-stack等
                  std::string_view name = this->shstrtab.data() + shdr.sh_name;
                  // 简单创建了一个InputSection
                  this->sections[i] = std::make_unique<InputSection<E>>(ctx, *this, i);
          }
      }
      // Attach relocation sections to their target sections. 将重新定位部分附加到其目标段
      // 针对RELA和REL处理，设置上对应的relsec_idx
      for (i64 i = 0; i < this->elf_sections.size(); i++) {
          const ElfShdr<E> &shdr = this->elf_sections[i];
          if (shdr.sh_type != (E::is_rela ? SHT_RELA : SHT_REL))
            continue;
  
          if (shdr.sh_info >= sections.size())
            Fatal(ctx) << *this << ": invalid relocated section index: "
                       << (u32)shdr.sh_info;
  
          if (std::unique_ptr<InputSection<E>> &target = sections[shdr.sh_info]) {
            assert(target->relsec_idx == -1);
            target->relsec_idx = i;
          }
    }
  }
  ```

###### initialize_symbols :ok:

- elf/input-files.cc

- 这部分的过程主要是将esym转换为Symbol。esym则是ElfSym的缩写，也就是Elf文件中的Symbol定义，而Symbol则是mold中自己定义的，相当于转换为自己想要的格式。

- 这里的symtab_sec是parse刚开始的时候寻找的section

- ```c++
  template <typename E>
  void ObjectFile<E>::initialize_symbols(Context<E> &ctx) {
      if (!symtab_sec)
          return;
  
      static Counter counter("all_syms");
      counter += this->elf_syms.size();
  
      // Initialize local symbols 以及第0个符号
      this->local_syms.resize(this->first_global);
      this->local_syms[0].file = this;
      this->local_syms[0].sym_idx = 0;
      
      // 处理local符号 直到第一个global符号
      for (i64 i = 1; i < this->first_global; i++) {
          const ElfSym<E> &esym = this->elf_syms[i];
          if (esym.is_common())
              Fatal(ctx) << *this << ": common local symbol?";
  
          std::string_view name;
          if (esym.st_type == STT_SECTION)
              name = this->shstrtab.data() + this->elf_sections[get_shndx(esym)].sh_name;
          else
              // 取得符号的名称
              name = this->symbol_strtab.data() + esym.st_name;
  		// 将本地local符号转为mold 定义的Symbol
          Symbol<E> &sym = this->local_syms[i];
          sym.set_name(name);
          sym.file = this;
          sym.value = esym.st_value;
          sym.sym_idx = i;
          
          if (!esym.is_abs())
       		// 如果不是绝对段 .text       
              sym.set_input_section(sections[get_shndx(esym)].get());
      }
      // 两个resize容器的位置
      this->symbols.resize(this->elf_syms.size());
      
      i64 num_globals = this->elf_syms.size() - this->first_global;
      has_symver.resize(num_globals);
      
      // 将local_sym绑定到symbols中
      for (i64 i = 0; i < this->first_global; i++)
          this->symbols[i] = &this->local_syms[i];
      // Initialize global symbols
      for (i64 i = this->first_global; i < this->elf_syms.size(); i++) {
          // 取到全局符号
          const ElfSym<E> &esym = this->elf_syms[i];
  
          // Get a symbol name
          std::string_view key = this->symbol_strtab.data() + esym.st_name;
          std::string_view name = key;
  
          // Parse symbol version after atsign
          if (i64 pos = name.find('@'); pos != name.npos) {
            std::string_view ver = name.substr(pos);
            name = name.substr(0, pos);
  
            if (ver != "@" && ver != "@@") {
              if (ver.starts_with("@@"))
                key = name;
              has_symver.set(i - this->first_global);
            }
          }
          // 插入符号
          this->symbols[i] = insert_symbol(ctx, esym, key, name);
          if (esym.is_common())
            has_common_symbol = true;
      }
  }
  ```

  - SHN_ABS This value specifies absolute values for the corresponding reference. For example, symbols defined relative to section number SHN_ABS have **absolute values and are not affected by relocation.**

  - 非abs符号，也就是说都是相对地址，会affected by relocation。

  - 而实际set_input_section则是设置其mask位，用于区分什么性质的符号。

    - ```c++
      template <typename E>
      inline void Symbol<E>::set_input_section(InputSection<E> *isec) {
        uintptr_t addr = (uintptr_t)isec;
        assert((addr & TAG_MASK) == 0);
        origin = addr | TAG_ISEC;
      }
      ```

    - ```c++
      // A symbol usually belongs to an input section, but it can belong
      // to a section fragment, an output section or nothing
      // (i.e. absolute symbol). `origin` holds one of them. We use the
      // least significant two bits to distinguish type.
      enum : uintptr_t {
        TAG_ABS  = 0b00,
        TAG_ISEC = 0b01,
        TAG_OSEC = 0b10,
        TAG_FRAG = 0b11,
        TAG_MASK = 0b11,
      };
      ```

  - 对全局符号

    - 在开始处理之前可以看到这里又有两个resize容器的位置，目前为止有三处，这里写明了对应的容器以及所处的类，用于区分这个信息是否为ObjectFile only的

      1. local symbols(InputFile)
      2. symbols(InputFile)
      3. symvers (ObjectFile)

    - 这里不需要再区分是否为Section的符号，因为global符号不包含section符号。

    - 这里最主要的是需要解析symbol version，因为有的符号会依赖于版本号。要注意的是这个东西并非ELF的官方定义，而是GNU的一个扩展，因此去看elf specification是找不到的。关于名称规范也很简单，常规符号名后接@加符号版本

      解析符号版本完成后设置到symvers中

  - insert symbol，并且设置其common属性。要注意除了这些解析方式外，global symbol和local symbol相比还有一个比较隐藏的不同，global symbol没有设置对应的file，后面很多符号的处理会进行判断file

    - ```c++
      template <typename E>
      static Symbol<E> *insert_symbol(Context<E> &ctx, const ElfSym<E> &esym,
                                      std::string_view key, std::string_view name) {
        if (esym.is_undef() && name.starts_with("__real_") &&
            ctx.arg.wrap.contains(name.substr(7))) {
          return get_symbol(ctx, key.substr(7), name.substr(7));
        }
      
        Symbol<E> *sym = get_symbol(ctx, key, name);
      
        if (esym.is_undef() && sym->is_wrapped) {
          key = save_string(ctx, "__wrap_" + std::string(key));
          name = save_string(ctx, "__wrap_" + std::string(name));
          return get_symbol(ctx, key, name);
        }
        return sym;
      }
      ```

      - 这里不只是不存在key就创建key并返回那么简单。
        1. 关于save_string的问题，这里也是和之前一样，创建了string后由ctx来管理生命周期，返回一个string_view提供使用。
        2. 除此之外get_symbol的部分是实际执行了符号不存在则创建新符号并且返回的工作

###### sort_relocations :norway:

###### parse_ehframe :norway:

- 目前这两个没啥用对最简单的main函数

## create_internal_file :ok:

-  Create a dummy object file containing linker-synthesized symbols.

- ```c++
  template <typename E>
  void create_internal_file(Context<E> &ctx) {
    ObjectFile<E> *obj = new ObjectFile<E>;
    ctx.obj_pool.emplace_back(obj);
    ctx.internal_obj = obj;
    ctx.objs.push_back(obj);
  
    ctx.internal_esyms.resize(1);
  
    obj->symbols.push_back(new Symbol<E>);
    obj->first_global = 1;
    obj->is_alive = true;
    obj->priority = 1;
  ```

- internal file

  - 内部的文件，用来保存linker-synthesized符号。linker-synthesized符号或许也可以理解为编译产物中不存在的符号。作为一个并不实际存在的文件，依然会作为一个普通的ObjFile加入到obj_pool中，主要用途是在create_output_sections以后来add_synthetic_symbol，与之相关联的有一个internal_esyms，里面都是具体相关的符号。

  - 首先创建了基本的ObjectFile对象并且进行了一些初始化的处理。

    之后添加从命令行参数中读取的–defsym里的所有的defsym

    - 通过add和add_undef函数把defsym指定的符号添加到symbols中，并且设定为了特殊值，关联到了一个esym里。主要的差别就在于st_shndx被设置为了不同的值。

  - 处理完defsym后再从命令行参数中读取的SectionOrder的符号
  - 最后设置obj类的一些参数

## resolve_symbols :ok:

- 在mold中，这个部分做了四件事情
  1. 检测所有需要使用的objet files
  2. 移除重复的COMDAT段
  3. 进行符号决议的过程。在多个不同的esym中选择出一个更高priority的关联到sym中
  4. LTO的处理，处理后再次执行决议

- ```c++
  // resolve_symbols is 4 things in 1 phase:
    //
    // - Determine the set of object files to extract from archives.
    // - Remove redundant COMDAT sections (e.g. duplicate inline functions).
    // - Finally, the actual symbol resolution.
    // - LTO, which requires preliminary symbol resolution before running
    //   and a follow-up re-resolution after the LTO objects are emitted.
    //
    // These passes have complex interactions, and unfortunately has to be
    // put together in a single phase.
  ```

  ```c++
  template <typename E>
  void resolve_symbols(Context<E> &ctx) {
    Timer t(ctx, "resolve_symbols");
  
    std::vector<ObjectFile<E> *> objs = ctx.objs;
    std::vector<SharedFile<E> *> dsos = ctx.dsos;
  
    do_resolve_symbols(ctx);
  
    if (ctx.has_lto_object) {
    }
  }
  ```

### do_resolve_symbols :ok:

- ```c++
  template <typename E>
  void do_resolve_symbols(Context<E> &ctx) {
    auto for_each_file = [&](std::function<void(InputFile<E> *)> fn) {
      tbb::parallel_for_each(ctx.objs, fn);
      tbb::parallel_for_each(ctx.dsos, fn);
    };
      // 1. 检测object files
      // 2. 消除重复COMDAT
      // 3. 符号决议
  }
  ```

#### 检测object files

- ```c++
  {
      Timer t(ctx, "extract_archive_members");
  
      // Register symbols
      for_each_file([&](InputFile<E> *file) { file->resolve_symbols(ctx); });
  
      // Mark reachable objects to decide which files to include into an output.
      // This also merges symbol visibility.
      mark_live_objects(ctx);
  
      // Cleanup. The rule used for archive extraction isn't accurate for the
      // general case of symbol extraction, so reset the resolution to be redone
      // later.
      for_each_file([](InputFile<E> *file) { file->clear_symbols(); });
  
      // Now that the symbol references are gone, remove the eliminated files from
      // the file list.
      std::erase_if(ctx.objs, [](InputFile<E> *file) { return !file->is_alive; });
      std::erase_if(ctx.dsos, [](InputFile<E> *file) { return !file->is_alive; });
    }
  ```

  - for_each_file针对objs和dsos处理resolve，mark live，clear，erase file
  - 标记所有可访问的输出到文件的object，之后合并可见性
  - 清除file的symbols
  - 最后清除掉objs和dsos中非alive的file

- 实际进行符号决议的过程

- ```c++
  template <typename E>
  void ObjectFile<E>::resolve_symbols(Context<E> &ctx) {
    for (i64 i = this->first_global; i < this->elf_syms.size(); i++) {
      Symbol<E> &sym = *this->symbols[i];
      const ElfSym<E> &esym = this->elf_syms[i];
  
      if (esym.is_undef())
        continue;
  
      InputSection<E> *isec = nullptr;
      if (!esym.is_abs() && !esym.is_common()) {
        isec = get_section(esym);
        if (!isec || !isec->is_alive)
          continue;
      }
  
      std::scoped_lock lock(sym.mu);
      if (get_rank(this, esym, !this->is_alive) < get_rank(sym)) {
        sym.file = this;
        sym.set_input_section(isec);
        sym.value = esym.st_value;
        sym.sym_idx = i;
        sym.ver_idx = ctx.default_version;
        sym.is_weak = esym.is_weak();
        sym.is_imported = false;
        sym.is_exported = false;
      }
    }
  }
  ```

  - 符号决议是针对global symbol的elf_sym的。未定义的esym都跳过了，它们都不需要参与resolve的过程，因为resolve本质是找到需要加入到生成产物的符号实现，但是注意在前面mark的时候还是需要的。
  - 也就是说Symbol类的sym其实是保留的最终唯一定义。而在决议的过程，不断的将esym和对应的sym进行比较。如果esym的rank小，也就是更加优先，那么就将sym中的信息更新为对应的esym的信息，这就是实际决议过程中做的事情。而这里也是实际初始化symbols成员里global的值的地方，local的部分初始化在parse的阶段就做好了，因为local的符号并不需要进行resolve。

## 暂时没用 

### kill_eh_frame_sections:no_entry_sign:

/ "Kill" .eh_frame input sections after symbol resolution.

###  resolve_section_pieces:no_entry_sign:

- Resolve mergeable section pieces to merge them.

```c++
// Handle --relocatable. Since the linker's behavior is quite different

 // from the normal one when the option is given, the logic is implemented

 // to a separate file.

 if (ctx.arg.relocatable) {

  combine_objects(ctx);

  return 0;

 }
```



### convert_common_symbols:no_entry_sign:

- Create .bss sections for common symbols.

 convert_common_symbols(ctx);

### apply_version_script:no_entry_sign:

- Apply version scripts.

 apply_version_script(ctx);

### parse_symbol_version:no_entry_sign:

- 解析符号版本后缀 (e.g. "foo@ver1").

 parse_symbol_version(ctx);

## 创建输出段之前

### compute_merged_section_sizes :ok:

增加.comment段表示是Mold特有的

### create_synthetic_sections :ok:

- 创建链接器合成的段，如.got或.plt

- 这里主要创建一些特殊的段

- `phdr`和`ehdr`是与可执行文件格式相关的结构体。

  - `phdr`代表"Program Header"，也称为段头表。它是可执行文件中描述程序段的元数据的数据结构。每个程序段都有一个对应的`phdr`条目，用于描述段的起始地址、大小、访问权限等信息。`phdr`通常用于动态链接器（dynamic linker）在加载和链接可执行文件时确定程序段的位置和属性。


  - `ehdr`代表"ELF Header"，也称为ELF文件头。它是可执行文件中描述整个文件结构的元数据的数据结构。`ehdr`包含了诸如文件类型、机器架构、入口点地址、段头表的偏移量等信息，是ELF文件的起始部分。`ehdr`提供了解析和处理ELF文件的基本信息。

  - 这两个数据结构是在可执行文件格式中使用的关键结构，用于描述和组织可执行文件的内容和元数据。

- 在这里其实已经开始创建输出的内容了，因为是直接push到chunk中。在mold中chunk则是表示用于输出的一片区域，关于Chunk类源码中有这样的注释

### check_duplicate_symbols :no_entry_sign:

- 确定没有重复的符号

## 创建输出段

### create_output_sections :ok:

- 创造输出段
  - 首先针对所有的InputSection生成一个key，并且根据key创建所有的OutputSection
  - 将所有obj中的InputSection加入到对应OutputSection的members中
  - 对所有的output section和mergeable section加入到chunks
  - 将所有的chunk进行排序
  - 所有的chunk加入到ctx.chunks中（在加入之前chunks中有一些synthetic的chunk，在上一期中有提及）
  - 最简单的实例包括.bss .comment .text .data

#### get_output_section_key

- 这个函数的作用是从一个InputSection构造一个OutputSectionKey

### add_synthetic_symbols :ok: 

- 就是添加一些synthetic的符号，添加后将这些符号关联到ctx.symtab中

- 针对特殊名字的trunk的处理  就是之前添加的.got等特殊段

  - ```c++
    for (Chunk<E> *chunk : ctx.chunks) {
        if (std::optional<std::string> name = get_start_stop_name(ctx, *chunk)) {
          add(save_string(ctx, "__start_" + *name));
          add(save_string(ctx, "__stop_" + *name));
    
          if (ctx.arg.physical_image_base) {
            add(save_string(ctx, "__phys_start_" + *name));
            add(save_string(ctx, "__phys_stop_" + *name));
          }
        }
      }
    ```

- 对internal_obj进行symbol resolve

  - ```c++
    obj.elf_syms = ctx.internal_esyms;
      obj.has_symver.resize(ctx.internal_esyms.size() - 1);
    
      obj.resolve_symbols(ctx);
    ```

    

- 符号关联到symtab这个output section里

  - ```c++
    // Handle --defsym symbols.
    for (i64 i = 0; i < ctx.arg.defsyms.size(); i++) {
      Symbol<E> *sym = ctx.arg.defsyms[i].first;
      std::variant<Symbol<E> *, u64> val = ctx.arg.defsyms[i].second;
    
      Symbol<E> *target = nullptr;
      if (Symbol<E> **ref = std::get_if<Symbol<E> *>(&val))
        target = *ref;
    
      // If the alias refers another symobl, copy ELF symbol attributes.
      if (target) {
        ElfSym<E> &esym = obj.elf_syms[i + 1];
        esym.st_type = target->esym().st_type;
        if constexpr (requires { esym.ppc_local_entry; })
          esym.ppc_local_entry = target->esym().ppc_local_entry;
      }
    
      // Make the target absolute if necessary.
      if (!target || target->is_absolute())
        sym->origin = 0;
    }
    ```

- 符号关联到symtab这个output section里

  - ```c++
    // Make all synthetic symbols relative ones by associating them to
    // a dummy output section.
    for (Symbol<E> *sym : obj.symbols)
      if (sym->file == &obj)
        sym->set_output_section(ctx.symtab);
    ```

## 未解析符号的处理 :no_entry_sign:

### claim_unresolved_symbols

- 对未解析符号处理

- ```c++
  // If we are linking a .so file, remaining undefined symbols does
  // not cause a linker error. Instead, they are treated as if they
  // were imported symbols.
  //
  // If we are linking an executable, weak undefs are converted to
  // weakly imported symbols so that they'll have another chance to be
  // resolved.
  ```

- 这个函数主要还是针对需要在链接期就确定定义的符号进行检查，针对部分符号产生一些修改，在这个过程之后，不会再有符号发生新的变动了

- 对so来说undef是可以存在的，因此将避免报错，将undef的符号转换为imported，并且修改相关信息。

  但是如果是protected或者hidden的符号即便链接了运行时也无法访问到，此时即便是undef也无法再在运行时找到定义，因此需要在链接时确定定义。也正因为这些条件，这里只需要对global符号做检查即可。

#### claim_unresolved_symbols

如同上面所说，整个过程描述如下

1. 从全局符号开始，先跳过了已经有定义的esym

2. 将protected和hidden的符号进行报错

3. 对esym对应位置的sym进行判断，如果sym所对应的esym是有定义的也跳过。

   这种情况是esym实际的定义在其他位置，sym是esym resolve的结果

4. 解析符号名，如果带有版本信息则再次尝试进行重新将esym和sym进行关联。这个关联体现在esym对应index的symbols重新设置值

5. 针对undef_weak进行claim

6. 剩下的undef的符号在创建executable的时候导致链接失败，但在dso中会被提升为dynamic symbols

## 段排序

本篇文章提到的mold中出现的段排序，包含了一个chunk内的段与段的排序，还包含了chunk与chunk之间的排序。或者也可以说是对于输入角度来看待的排序，以及从输出角度看待的段进行排序。对于输入来讲，段的基本单位是InputSection，比如说一个输入文件中的一个text段，而对于输出来讲，也就是目标文件来讲，段的基本单位是一个chunk，而一个chunk是由多个输入的段组成的，比如说大的text段是由所有的输入文件中的text段组合而成。

- 首先要说明为什么需要进行排序
  - sort_init_fini，sort_ctor_dtor以及shuffle_sections属于chunk内的段与段之间的排序，在这里来说是为了满足mold的特殊需求。不过完全随机以及reverse的shuffle我还是不明白为什么需要这样来做。对于init这样的段来说，链接器需要将所有输入文件的同名段合并到同一个输出段中，因此必须要在chunk内的段与段之间的排序。
  - sort_output_sections属于chunk与chunk之间的排序，这里排序的目的很大一部分是为了满足特定规则的需要，不论是regular顺序的还是指定的顺序都会使得ehdr/phdr在最前，而shdr在最后。
  - 对于中间灵活可变的部分，和对齐以及跳转指令都有关系。跳转指令这个问题我在实际遇到过，JAL指令的立即数字段的长度是固定的，而所要跳转的地址超出了JAL这个字段所能代表的长度，最后通过修改链接脚本中相关段的顺序使得地址控制在了立即数的范围之内。

### sort_init_fini

`.init_array`和`.fini_array`内容必须按一个特殊规则排序。

### sort_ctor_dtor:no_entry_sign:

### scan_relocations :no_entry_sign:

elf/passes.c

为了保证输出文件中的got和plt等section包含对应rel的信息，递归所有所有的rel找到在got和plt中需要的符号，并且添加到got/plt中。而got和plt本身就是synthetic的段，无法从编译产物中获取，只能在链接的时候产生，为了确保rel段符号的正确查找，一定需要这一步骤。

这里主要做了三部分

1. 扫描每个obj里所有段中的符号，另外标记rel段中要处理的符合条件的符号为NEEDS_PLT
2. 将flag不为空，或者是imported/exported的符号保留，过滤掉其他符号
3. 对过滤后的符号，根据其flga添加到对应的chunk中，比如说got或者plt等，最后再清空其flag

#### rel scan

#### symbols to a vec

这里将所有的文件中所有符合条件的symbol收集起来，其中flag则是在前面的阶段进行标记的

#### add to table

将所有符合条件的符号添加到ctx.symbol_aux中，之后加入到对应的表中

### compute_section_sizes :yes

在分配偏移量时计算输出部分的大小在输出段内到输入段。

这里做了如下几件事情

1. chunks里面找到所有符合条件的osec进行处理
   1. 划分group并且处理每个group的size和p2align
   2. 计算与设置group的offset以及p2align
2. 处理ARM的特殊情况
3. 根据arg的section_align设定特定osec的sh_addralign

#### chunks的处理

```c++
template <typename E>
void compute_section_sizes(Context<E> &ctx) {
  Timer t(ctx, "compute_section_sizes");

  struct Group {
    i64 size = 0;
    i64 p2align = 0;
    i64 offset = 0;
    std::span<InputSection<E> *> members;
  };

	tbb::parallel_for_each(ctx.chunks, [&](Chunk<E> *chunk) {
    OutputSection<E> *osec = chunk->to_osec();
    if (!osec)
      return;

    // This pattern will be processed in the next loop.
    if constexpr (needs_thunk<E>)
      if ((osec->shdr.sh_flags & SHF_EXECINSTR) && !ctx.arg.relocatable)
        return;

    // Since one output section may contain millions of input sections,
    // we first split input sections into groups and assign offsets to
    // groups.
    std::vector<Group> groups;
    constexpr i64 group_size = 10000;

    for (std::span<InputSection<E> *> span : split(osec->members, group_size))
      groups.push_back(Group{.members = span});

    tbb::parallel_for_each(groups, [](Group &group) {
      for (InputSection<E> *isec : group.members) {
        group.size = align_to(group.size, 1 << isec->p2align) + isec->sh_size;
        group.p2align = std::max<i64>(group.p2align, isec->p2align);
      }
    });

    i64 offset = 0;
    i64 p2align = 0;

    for (i64 i = 0; i < groups.size(); i++) {
      offset = align_to(offset, 1 << groups[i].p2align);
      groups[i].offset = offset;
      offset += groups[i].size;
      p2align = std::max(p2align, groups[i].p2align);
    }

    osec->shdr.sh_size = offset;
    osec->shdr.sh_addralign = 1 << p2align;

    // Assign offsets to input sections.
    tbb::parallel_for_each(groups, [](Group &group) {
      i64 offset = group.offset;
      for (InputSection<E> *isec : group.members) {
        offset = align_to(offset, 1 << isec->p2align);
        isec->offset = offset;
        offset += isec->sh_size;
      }
    });
  });
```

1. 找到chunks中的osec
2. 将osec中的InputSections拆分为几个group并行计算
3. 针对每个group的每个InputSection
   1. 将group size根据isec的p2align计算出一个对齐的size，之后加上当前isec的size
   2. p2align更新为最大值
4. 更新所有group的offset以及p2align
5. 更新osec的size和addralign
6. 对所有group中的input section设置offset

#### ARM的处理

```c++
template <typename E>
void compute_section_sizes(Context<E> &ctx) {
	...

	// On ARM32 or ARM64, we may need to create so-called "range extension
  // thunks" to extend branch instructions reach, as they can jump only
  // to ±16 MiB or ±128 MiB, respecitvely.
  //
  // In the following loop, We compute the sizes of sections while
  // inserting thunks. This pass cannot be parallelized. That is,
  // create_range_extension_thunks is parallelized internally, but the
  // function itself is not thread-safe.
  if constexpr (needs_thunk<E>) {
    Timer t2(ctx, "create_range_extension_thunks");

    if (!ctx.arg.relocatable)
      for (Chunk<E> *chunk : ctx.chunks)
        if (OutputSection<E> *osec = chunk->to_osec())
          if (osec->shdr.sh_flags & SHF_EXECINSTR)
            osec->create_range_extension_thunks(ctx);
  }

  ...
}
```

- .text段

### sort_output_sections :ok:

按节属性排序，我们需要创建尽可能少的段

这里对所有chunk进行了排序。

其中顺序为

| ELF Header<br/>program header<br/>normal memory allocated sections<br/>non-memory-allocated sections<br/>section header |
| ------------------------------------------------------------ |

normal memory allocated sections的顺序会根据用户指定的顺序，或者使用一套regular的规则

#### regular

- 注释中对排序的规则进行了说明

  - ```c++
    // We want to sort output chunks in the following order.
    //
    //   <ELF header>
    //   <program header>
    //   .interp
    //   .note
    //   .hash
    //   .gnu.hash
    //   .dynsym
    //   .dynstr
    //   .gnu.version
    //   .gnu.version_r
    //   .rela.dyn
    //   .rela.plt
    //   <readonly data>
    //   <readonly code>
    //   <writable tdata>
    //   <writable tbss>
    //   <writable RELRO data>
    //   .got
    //   .toc
    //   <writable RELRO bss>
    //   .relro_padding
    //   <writable non-RELRO data>
    //   <writable non-RELRO bss>
    //   <non-memory-allocated sections>
    //   <section header>
    ```

### relr and dynsym

#### dynsym finalize

- 为dynamic symbol的字符串在dynstr中留出空间，并且排序dynsym的内容。在这之后不会有符号被加入到dynsym，因此这里dynstr section的大小以及排布确定下来了。

  具体的处理过程如下

  1. symbols排序，local在前global在后，和elf中的格式一样。
  2. 处理gnu_hash的情况
  3. 设置dynsym_offset后计算dynstr的size
  4. 更新DynsymSection的shdr的信息

#### report_undef_errors

- 报告之前在claim_unresolved_symbols中收集的undef的错误信息

## 创建一些输出段

### ctx.verneed->construct(ctx) :no_entry_sign:

- Fill .gnu.version_r section contents.

### create_output_symtab :ok:

- Compute .symtab and .strtab sizes for each file.
- 这个过程是用于创建symtab和strtab，创建的时候会实际选择哪些符号要写到文件中。我们熟悉的strip，如果添加了链接选项那么就是在这里开始生效的。

#### chunk

```c++
// Chunk::compute_symtab_size

// Some synethetic sections add local symbols to the output.
// For example, range extension thunks adds function_name@thunk
// symbol for each thunk entry. The following members are used
// for such synthesizing symbols.
virtual void compute_symtab_size(Context<E> &ctx) {};
```

```markdown
对于chunk来说，不是所有的都需要做这一步操作的。在mold中仅针对OutputSection，Got，Plt，PltGot这几个chunk来处理。

实际要做的就是遍历所有符号更新其strtab_size以及num_local_symtab（用于标记local符号的数量，也就是这个阶段要计算的symtab size），不论是哪一种chunk都是如此，下面就不再赘述，只贴代码了。
```

#### obj

在obj中，主要计算了local和global符号的名字占用的空间，用于更新strtable_size，另外还会更新对应的output_sym_indices

要注意的是计算名字空间的时候，这里的名字需要使用null结尾，因此size还需要加一。

对于local symbol除了要判断alive之外，还有一个should_write_to_local_symtab的判断，除了更新size外还会更新write_to_symtab



### eh_frame_construct :ok:

```c++
// .eh_frame is a special section from the linker's point of view,
// as its contents are parsed and reconstructed by the linker,
// unlike other sections that are regarded as opaque bytes.
// Here, we construct output .eh_frame contents.
ctx.eh_frame->construct(ctx);
```

## 计算shdr以及osec offset 

`shdr`代表"Section Header"，也称为节头表

`osec_offset`代表"output section"，输出节

本期的内容主要是更新section header以及set output section offsets相关。当这些操作结束后，虚拟地址会固定，因此输出文件的memory layout就固定下来了。

### compute_section_headers :ok:

这里主要做了这么几件事

1. 所有输出段更新shdr
2. 移除所有空的chunk
3. 重新设置所有chunk的index（因为上面移除了chunk，index发生了改变）
4. SymtabShndxSection的处理
5. 再次更新所有的shdr

#### update_shdr

#### 移除空的chunk

#### 重新更新索引

- e_shnum和e_shstrndx是在EHDR中的信息。首先是shnum，当shdr table，也就是说section的数量超过SHN_LORESERVE的时候，e_shnum会设置为0，实际的数量会保存在shdr table的初始条目中的sh_size的字段里，其他情况这个条目的sh_size的字段是0。

- 这里创建了一个SymtabShndxSection，也就是”symtab_shndx”段，这个段保留了特殊的symbol table section index arrry，指向与符号表关联的shdr的索引。

  - `.symtab_shndx`

    This section holds the special symbol table section index array, as described above. The section’s attributes will include the `SHF_ALLOC` bit if the associated symbol table section does; otherwise that bit will be off.

  - `SHT_SYMTAB_SHNDX`

    This section is associated with a section of type `SHT_SYMTAB` and is required if any of the section header indexes referenced by that symbol table contain the escape value `SHN_XINDEX`. The section is an array of `Elf32_Word` values. Each value corresponds one to one with a symbol table entry and appear in the same order as those entries. The values represent the section header indexes against which the symbol table entries are defined. Only if corresponding symbol table entry’s `st_shndx` field contains the escape value `SHN_XINDEX` will the matching `Elf32_Word` hold the actual section header index; otherwise, the entry must be `SHN_UNDEF` (`0`).

### set_osec_offsets :ok:

设置所有output section的offset，根据是否有section_order会选择不同的排列方式，导致output的offset是不同的。

为输出段分配虚拟地址和文件偏移量。

主要分为如下几部分

1. 设置段的虚拟地址
2. 设置段在文件中的offset
3. 处理phdr并且返回文件的长度

#### 虚拟地址设置规则

mold中不论是指定order还是不指定都是遵循基本的两条规则

1. memory protection: 不同attr的section分配到不同的页中，为了满足每个段只有一个attr的条件
2. ELF spec requires: vaddr的地址的模要是page size，这里会在设置地址的时候进行align，也就是insert padding。

这里提及的ticks:

1. place sections with the same memory attributes contiguous as possible. 相同attr尽可能连续。（因为不同attr需要放入不同的页）
2. map the same file region to memory more than once. map两次，因此不会节约内存空间，但是会节约磁盘空间

## 固定文件layout以及创建输出

### fix_synthetic_symbols :ok:

## 最后的收尾

