/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "write.h"
#include "utils.h"
#include "global.h"
#include "strdefs.h"
#include "error.h"
#include <stdlib.h>

void
WriteIncludes(file, isuser, isdef)
    FILE *file;
    boolean_t	isuser;
    boolean_t   isdef;
{
    if (isdef) {
	fprintf(file, "#include <mach/port.h>\n");
	fprintf(file, "#include <mach/machine/kern_return.h>\n");
	if (!isuser)
	    fprintf(file, "#include <mach/mig_errors.h>\n");
    }
    else {
        fprintf(file, "#include <string.h>\n");
        fprintf(file, "#include <mach/ndr.h>\n");
        fprintf(file, "#include <mach/boolean.h>\n");
        fprintf(file, "#include <mach/kern_return.h>\n");
        fprintf(file, "#include <mach/notify.h>\n");
        fprintf(file, "#include <mach/mach_types.h>\n");
        fprintf(file, "#include <mach/message.h>\n");
	fprintf(file, "#include <mach/mig_errors.h>\n");
        if (ShortCircuit)
	        fprintf(file, "#include <mach/rpc.h>\n");
        if (isuser && IsKernelUser) {
	    fprintf(file, "#if _MIG_KERNEL_SPECIFIC_CODE_\n");
	    fprintf(file, "#include <kern/ipc_mig.h>\n");
	    fprintf(file, "#endif /* _MIG_KERNEL_SPECIFIC_CODE_ */\n");
	    fprintf(file, "#include <mach/port.h>\n");
	} else
	  fprintf(file, "#include <mach/port.h>\n");
    }
    fprintf(file, "\n");
}

static void
WriteETAPDefines(file)
    FILE *file;
{
    register statement_t *stat;
    int fnum;
    char *fname;
    int first = TRUE;

    fprintf(file, "\n#ifndef subsystem_to_name_map_%s\n", SubsystemName);
    fprintf(file, "#define subsystem_to_name_map_%s \\\n", SubsystemName);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
        if (stat->stKind == skRoutine)
	{
	    fnum = SubsystemBase + stat->stRoutine->rtNumber;
	    fname = stat->stRoutine->rtName;
	    if (! first)
		fprintf(file, ",\\\n");
	    fprintf(file, "    { \"%s\", %d }", fname, fnum);
	    first = FALSE;
	}
    fprintf(file, "\n#endif\n");
}

static void
WriteDefines(file)
    FILE *file;
{
}

static void
WriteMigExternal(file)
    FILE *file;
{
    fprintf(file, "#ifdef\tmig_external\n");
    fprintf(file, "mig_external\n");
    fprintf(file, "#else\n");
    fprintf(file, "extern\n");
    fprintf(file, "#endif\t/* mig_external */\n");
}

static void
WriteProlog(file, protect, more, isuser)
    FILE *file;
    char *protect;
    boolean_t more, isuser;
{
    if (protect != strNULL) {
	fprintf(file, "#ifndef\t_%s\n", protect);
	fprintf(file, "#define\t_%s\n", protect);
	fprintf(file, "\n");
    }

    fprintf(file, "/* Module %s */\n", SubsystemName);
    fprintf(file, "\n");

    if (more) {
	WriteIncludes(file, isuser, UseSplitHeaders);
	WriteDefines(file);
    }
    fprintf(file, "#ifdef AUTOTEST\n");
    fprintf(file, "#ifndef FUNCTION_PTR_T\n");
    fprintf(file, "#define FUNCTION_PTR_T\n");
    fprintf(file, "typedef void (*function_ptr_t)");
    fprintf(file, "(mach_port_t, char *, mach_msg_type_number_t);\n");
    fprintf(file, "typedef struct {\n");
    fprintf(file, "        char            *name;\n");
    fprintf(file, "        function_ptr_t  function;\n");
    fprintf(file, "} function_table_entry;\n");
    fprintf(file, "typedef function_table_entry 	*function_table_t;\n");
    fprintf(file, "#endif /* FUNCTION_PTR_T */\n");
    fprintf(file, "#endif /* AUTOTEST */\n");
    fprintf(file, "\n#ifndef\t%s_MSG_COUNT\n", SubsystemName);
    fprintf(file, "#define\t%s_MSG_COUNT\t%d\n", SubsystemName, rtNumber);
    fprintf(file, "#endif\t/* %s_MSG_COUNT */\n\n", SubsystemName);
}

static void
WriteEpilog(file, protect, isuser)
    FILE *file;
    char *protect;
    boolean_t isuser;
{
    char *defname = 
        isuser ? "__AfterMigUserHeader" : "__AfterMigServerHeader";

    WriteETAPDefines(file);
    fprintf(file, "\n#ifdef %s\n%s\n#endif /* %s */\n",
	    defname, defname, defname);
    if (protect != strNULL) {
	fprintf(file, "\n");
	fprintf(file, "#endif\t /* _%s */\n", protect);
    }
}

static void
WriteUserRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    WriteMigExternal(file);
    fprintf(file, "%s %s\n", ReturnTypeStr(rt), rt->rtUserName);
    if (BeLint) {
      fprintf(file, "#if\t%s\n", LintLib);
      fprintf(file, "    (");
      WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", " , "");
      fprintf(file, ")\n");
      WriteList(file, rt->rtArgs, WriteUserVarDecl, akbUserArg, ";\n", ";\n");
      fprintf(file, "{ ");
      fprintf(file, "return ");
      fprintf(file, "%s(", rt->rtUserName);
      WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", ", "");
      fprintf(file, "); }\n");
      fprintf(file, "#else\n");
    }
    if (BeAnsiC) {
        fprintf(file, "(\n");
        WriteList(file, rt->rtArgs, WriteUserVarDecl, akbUserArg, ",\n", "\n");
        fprintf(file, ");\n");
    } else {
        fprintf(file, "#if\t%s\n", NewCDecl);
        fprintf(file, "(\n");
        WriteList(file, rt->rtArgs, WriteUserVarDecl, akbUserArg, ",\n", "\n");
        fprintf(file, ");\n");
        fprintf(file, "#else\n");

        fprintf(file, "    ();\n");
        fprintf(file, "#endif\t/* %s */\n", NewCDecl); 
    }
    if (BeLint) {
      fprintf(file, "#endif\t/* %s */\n",  LintLib);
    }
}

void
WriteUserRequestUnion(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;

    fprintf(file, "/* union of all requests */\n\n");
    fprintf(file, "#ifndef __RequestUnion__%s%s_subsystem__defined\n", UserPrefix, SubsystemName);
    fprintf(file, "#define __RequestUnion__%s%s_subsystem__defined\n", UserPrefix, SubsystemName);
    fprintf(file, "union __RequestUnion__%s%s_subsystem {\n", UserPrefix, SubsystemName);
    for (stat = stats; stat != stNULL; stat = stat->stNext) {
        if (stat->stKind == skRoutine) {
            register routine_t *rt;
	    
	    rt = stat->stRoutine;
	    fprintf(file, "\t__Request__%s_t Request_%s;\n",
		    rt->rtName, rt->rtUserName);
	}
    }
    fprintf(file, "};\n");
    fprintf(file, "#endif /* !__RequestUnion__%s%s_subsystem__defined */\n", UserPrefix, SubsystemName);
}

void
WriteUserReplyUnion(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;

    fprintf(file, "/* union of all replies */\n\n");
    fprintf(file, "#ifndef __ReplyUnion__%s%s_subsystem__defined\n", UserPrefix, SubsystemName);
    fprintf(file, "#define __ReplyUnion__%s%s_subsystem__defined\n", UserPrefix, SubsystemName);
    fprintf(file, "union __ReplyUnion__%s%s_subsystem {\n", UserPrefix, SubsystemName);
    for (stat = stats; stat != stNULL; stat = stat->stNext) {
        if (stat->stKind == skRoutine) {
            register routine_t *rt;
	    
	    rt = stat->stRoutine;
	    fprintf(file, "\t__Reply__%s_t Reply_%s;\n",
		    rt->rtName, rt->rtUserName);
	}
    }
    fprintf(file, "};\n");
    fprintf(file, "#endif /* !__RequestUnion__%s%s_subsystem__defined */\n", UserPrefix, SubsystemName);
}

void
WriteUserHeader(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;
    char *protect = strconcat(SubsystemName, "_user_");

    WriteProlog(file, protect, TRUE, TRUE);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skImport:
	  case skUImport:
          case skDImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skRoutine:
	  case skSImport:
          case skIImport:
	    break;
	  default:
	    fatal("WriteHeader(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    fprintf(file, "\n#ifdef __BeforeMigUserHeader\n");
    fprintf(file, "__BeforeMigUserHeader\n");
    fprintf(file, "#endif /* __BeforeMigUserHeader */\n\n");
    for (stat = stats; stat != stNULL; stat = stat->stNext) {
	if (stat->stKind == skRoutine)
	    WriteUserRoutine(file, stat->stRoutine);
    }
    WriteRequestTypes(file, stats);
    WriteUserRequestUnion(file, stats);
    WriteReplyTypes(file, stats);
    WriteUserReplyUnion(file, stats);
    WriteEpilog(file, protect, TRUE);
}

static void
WriteDefinesRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    register char *up = (char *)malloc(strlen(rt->rtName)+1);

    up = toupperstr(strcpy(up, rt->rtName));
    fprintf(file, "#define\tMACH_ID_%s\t\t%d\t/* %s() */\n", 
	up, rt->rtNumber + SubsystemBase, rt->rtName);
    if (rt->rtKind == rkRoutine)
	fprintf(file, "#define\tMACH_ID_%s_REPLY\t\t%d\t/* %s() */\n", 
	    up, rt->rtNumber + SubsystemBase + 100, rt->rtName);
    fprintf(file, "\n");
}

void
WriteServerRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    WriteMigExternal(file);
    fprintf(file, "%s %s\n", ReturnTypeStr(rt), rt->rtServerName);
    if (BeLint) {
      fprintf(file, "#if\t%s\n", LintLib);
      fprintf(file, "    (");
      WriteList(file, rt->rtArgs, WriteNameDecl, akbServerArg, ", " , "");
      fprintf(file, ")\n");
      WriteList(file, rt->rtArgs, WriteServerVarDecl,
		akbServerArg, ";\n", ";\n");
      fprintf(file, "{ ");
      fprintf(file, "return ");
      fprintf(file, "%s(", rt->rtServerName);
      WriteList(file, rt->rtArgs, WriteNameDecl, akbServerArg, ", ", "");
      fprintf(file, "); }\n");
      fprintf(file, "#else /* %s */\n",LintLib);
    }
    if (BeAnsiC) {
        fprintf(file, "(\n");
        WriteList(file, rt->rtArgs, WriteServerVarDecl,
	      akbServerArg, ",\n", "\n");
        fprintf(file, ");\n");
    } else {
        fprintf(file, "#if\t%s\n", NewCDecl);
        fprintf(file, "(\n");
        WriteList(file, rt->rtArgs, WriteServerVarDecl,
	      akbServerArg, ",\n", "\n");
        fprintf(file, ");\n");
        fprintf(file, "#else\n");

        fprintf(file, "    ();\n");
        fprintf(file, "#endif\t/* %s */\n", NewCDecl); 
    }
    if (BeLint) {
      fprintf(file, "#endif\t/* %s */\n",  LintLib);
    }
}

static void
WriteDispatcher(file)
    FILE *file;
{
    register statement_t *stat;
    int descr_count = 0;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine)
	{
	    register routine_t *rt = stat->stRoutine;
	    descr_count += rtCountArgDescriptors(rt->rtArgs, (int *) 0);
	}
    fprintf(file, "\n");
    fprintf(file, "extern boolean_t %s(\n", ServerDemux);
    fprintf(file, "\t\tmach_msg_header_t *InHeadP,\n");
    fprintf(file, "\t\tmach_msg_header_t *OutHeadP);\n\n");

    fprintf(file, "extern mig_routine_t %s_routine(\n", ServerDemux);
    fprintf(file, "\t\tmach_msg_header_t *InHeadP);\n\n");

    fprintf(file, "\n/* Description of this subsystem, for use in direct RPC */\n");
    fprintf(file, "extern const struct %s {\n", ServerSubsys);
    if (UseRPCTrap) {
        fprintf(file, "\tstruct subsystem *\tsubsystem;\t/* Reserved for system use */\n");
    } else {
        fprintf(file, "\tmig_server_routine_t\tserver;\t/* Server routine */\n");
    }
    fprintf(file, "\tmach_msg_id_t\tstart;\t/* Min routine number */\n");
    fprintf(file, "\tmach_msg_id_t\tend;\t/* Max routine number + 1 */\n");
    fprintf(file, "\tunsigned int\tmaxsize;\t/* Max msg size */\n");
    if (UseRPCTrap) {
        fprintf(file, "\tvm_address_t\tbase_addr;\t/* Base address */\n");
        fprintf(file, "\tstruct rpc_routine_descriptor\t/*Array of routine descriptors */\n");
    } else {
        fprintf(file, "\tvm_address_t\treserved;\t/* Reserved */\n");
        fprintf(file, "\tstruct routine_descriptor\t/*Array of routine descriptors */\n");
    }
    fprintf(file, "\t\troutine[%d];\n", rtNumber);
    if (UseRPCTrap) {
	fprintf(file, "\tstruct rpc_routine_arg_descriptor\t/*Array of arg descriptors */\n");
	fprintf(file, "\t\targ_descriptor[%d];\n", descr_count);
    }
    fprintf(file, "} %s;\n", ServerSubsys);
    fprintf(file, "\n");
}

void
WriteServerHeader(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;
    char *protect = strconcat(SubsystemName, "_server_");

    WriteProlog(file, protect, TRUE, FALSE);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skImport:
	  case skSImport:
	  case skDImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skRoutine:
	  case skUImport:
	  case skIImport:
	    break;
	  default:
	    fatal("WriteServerHeader(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    fprintf(file, "\n#ifdef __BeforeMigServerHeader\n");
    fprintf(file, "__BeforeMigServerHeader\n");
    fprintf(file, "#endif /* __BeforeMigServerHeader */\n\n"); 
    WriteRequestTypes(file, stats);
    WriteServerRequestUnion(file, stats);
    WriteReplyTypes(file, stats);
    WriteServerReplyUnion(file, stats);
    for (stat = stats; stat != stNULL; stat = stat->stNext) {
	if (stat->stKind == skRoutine)
	    WriteServerRoutine(file, stat->stRoutine);
    }
    WriteDispatcher(file);
    WriteEpilog(file, protect, FALSE);
}

static void
WriteInternalRedefine(file, rt)
    FILE *file;
    register routine_t *rt;
{
    fprintf(file, "#define %s %s_external\n",
	    rt->rtUserName, rt->rtUserName);
}

void
WriteInternalHeader(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    WriteInternalRedefine(file, stat->stRoutine);
	    break;
	  case skImport:
	  case skUImport:
	  case skSImport:
	  case skDImport:
	  case skIImport:
	    break;
	  default:
	    fatal("WriteInternalHeader(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
}

void
WriteDefinesHeader(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;
    char *protect = strconcat(SubsystemName, "_defines");

    WriteProlog(file, protect, FALSE, FALSE);
    fprintf(file, "\n/*\tDefines related to the Subsystem %s\t*/\n\n", SubsystemName);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    WriteDefinesRoutine(file, stat->stRoutine);
	    break;
	  case skImport:
	  case skSImport:
	  case skUImport:
	    break;
	  default:
	    fatal("WriteDefinesHeader(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    WriteEpilog(file, protect, FALSE);
}
