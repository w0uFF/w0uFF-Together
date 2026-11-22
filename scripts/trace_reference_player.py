#@category w0uFF
# Read-only, reference-specific Ghidra 11.4.3 Jython script.
# Requires matching working DLL SHA256 b9055133ce5c28397e3fcd285cc4ba30268da0bfc0c1e82d5555ecf0aff0ac4f
# Prints only call graph and string reference observations, not source or full decompiled C.
from ghidra.app.decompiler import DecompInterface
from ghidra.program.util import DefinedDataIterator
from java.io import BufferedWriter, OutputStreamWriter, FileOutputStream

args = getScriptArgs()
if len(args) < 1 or len(args) > 2:
    raise ValueError("Output path and optional smoke/reference mode required")
mode = args[1] if len(args) == 2 else "reference"
if mode not in ("smoke", "reference"):
    raise ValueError("Unsupported mode")
writer = BufferedWriter(OutputStreamWriter(FileOutputStream(args[0]), "UTF-8"))

def emit(txt):
    writer.write(unicode(txt))
    writer.write(u"\n")

try:
    emit("w0uFF - reference DLL reverse engineering observations")
    emit("OBSERVATIONAL ONLY. Reconstructed symbols/calls are not original source.")
    emit("Executable: " + currentProgram.getName())
    emit("Analysis mode: " + mode)
    emit("Image base: " + str(currentProgram.getImageBase()))
    emit("Functions discovered: " + str(currentProgram.getFunctionManager().getFunctionCount()))
    if mode == "reference" and str(currentProgram.getImageBase()).lower() != "180000000":
        raise RuntimeError("Unexpected image base, cannot apply the reference addresses")

    # These addresses have been identified in the exact user-supplied binary,
    # but alone do not identify the player acquisition logic.
    fly_sites = [
        ("Fly UI string reference A", "180062ba0"),
        ("Fly UI string reference B", "180063fc4"),
    ]
    data_sites = [
        ("Fly Mode", "180cd5908"),
        ("Fly Mode##2", "180cd5b40"),
    ]
    targets = {}
    for label, hexaddr in (fly_sites if mode == "reference" else []):
        addr = toAddr(hexaddr)
        f = getFunctionContaining(addr)
        if f:
            targets[str(f.getEntryPoint())] = f
            emit("UI_SITE %s at %s function %s" % (label, addr, f.getEntryPoint()))
        else:
            emit("UNRESOLVED_SITE %s %s" % (label, hexaddr))

    for label, hexaddr in (data_sites if mode == "reference" else []):
        addr = toAddr(hexaddr)
        emit("STRING_SITE %s at %s" % (label, addr))
        for ref in getReferencesTo(addr):
            caller = getFunctionContaining(ref.getFromAddress())
            emit("STRING_XREF %s from %s function %s" % (label, ref.getFromAddress(),
                 caller.getEntryPoint() if caller else "(none)"))
            if caller:
                targets[str(caller.getEntryPoint())] = caller

    # Ghidra's defined strings are a *lead*, not proof of API calls.
    terms = ["SetFlyMode", "GetPlayerController", "GetPlayerPawn",
             "LocalPlayers", "GetPlayerCharacter", "IsLocalPlayerController"]
    matching_string_addresses = {}
    scanned = 0
    try:
        # Only known Fly-site xrefs are necessary for the first reference pass.
        # Full definedStrings iteration on a 15MB third-party PE can OOM Ghidra.
        iterator = [] if mode == "reference" else DefinedDataIterator.definedStrings(currentProgram)
        for data in iterator:
            scanned += 1
            if scanned > 150000:
                emit("STRING_SCAN_LIMIT")
                break
            try:
                value = unicode(data.getValue())
            except Exception:
                continue
            for term in terms:
                if term.lower() in value.lower() and len(value) < 200:
                    entry = str(data.getAddress())
                    if entry not in matching_string_addresses:
                        matching_string_addresses[entry] = term
                    if len(matching_string_addresses) >= 100:
                        break
            if len(matching_string_addresses) >= 100:
                break
    except Exception as e:
        # Bad/missing Ghidra string iterator must not block Fly-site analysis.
        emit("STRING_SCAN_ERROR " + str(e))
    writer.flush()

    emit("STRINGS_SCANNED %d (reference mode uses verified Fly xrefs instead)" % scanned)
    for entry, term in sorted(matching_string_addresses.items()):
        addr = toAddr(entry)
        refs = getReferencesTo(addr)
        emit("KEYWORD_STRING %s at %s" % (term, entry))
        ref_count = 0
        for ref in refs:
            if ref_count >= 10:
                break
            ref_count += 1
            caller = getFunctionContaining(ref.getFromAddress())
            emit("KEYWORD_XREF %s %s function %s" % (
                 term, ref.getFromAddress(),
                 caller.getEntryPoint() if caller else "(none)"))
            if caller:
                targets[str(caller.getEntryPoint())] = caller

    if mode == "smoke":
        for function in currentProgram.getFunctionManager().getFunctions(True):
            if not function.isExternal() and function.getBody().getNumAddresses() >= 32:
                targets[str(function.getEntryPoint())] = function
                break
    dec = DecompInterface()
    dec.openProgram(currentProgram)
    seen = set()
    queue = [(f, 0) for f in targets.values()][:4]
    emitted = 0
    for f, depth in queue:
        key = str(f.getEntryPoint())
        if key in seen:
            continue
        seen.add(key)
        emitted += 1
        if emitted > 12:
            emit("FUNCTION_LIMIT")
            break
        emit("FUNCTION %s name=%s size=%s depth=%d" % (
            key, f.getName(), f.getBody().getNumAddresses(), depth))
        callee_count = 0
        try:
            callees = sorted(list(f.getCalledFunctions(monitor)), key=lambda x:str(x.getEntryPoint()))
            for callee in callees:
                if callee_count >= 80:
                    emit("CALLEE_CAP")
                    break
                callee_count += 1
                emit("  CALL %s %s" % (callee.getEntryPoint(), callee.getName()))
        except Exception as e:
            emit("  CALL_GRAPH_ERROR " + str(e))

        # We do not export decompiled C from a third-party binary into a public artifact.
        # Keyword presence in reconstructed expressions only, not a claim of actual calls.
        if depth == 0:
            result = dec.decompileFunction(f, 35, monitor)
            if result.decompileCompleted():
                df = result.getDecompiledFunction()
                pseudo = df.getC() if df else ""
                found = [term for term in terms if term.lower() in pseudo.lower()]
                emit("  PSEUDOCODE_KEYWORDS " + ",".join(found))
                emit("  DECOMPILATION_COMPLETED")
            else:
                emit("  DECOMPILATION_ERROR " + str(result.getErrorMessage()))
    emit("REFERENCE_TRACE_DONE" if mode == "reference" else "SMOKE_TRACE_DONE")
finally:
    writer.close()
