#@category w0uFF
# Ghidra 11.4.x headless script, Jython 2-compatible.
# Read-only. Decompiler output is reconstruction, not original source.
# Arguments: output_path [smoke|reference]
from ghidra.app.decompiler import DecompInterface
from java.io import BufferedWriter, OutputStreamWriter, FileOutputStream

args = getScriptArgs()
output_path = args[0] if len(args) else "/tmp/wouff-ghidra.txt"
mode = args[1] if len(args) > 1 else "smoke"
writer = BufferedWriter(OutputStreamWriter(FileOutputStream(output_path), "UTF-8"))
def line(value):
    writer.write(unicode(value))
    writer.write(u"\n")

try:
    line("w0uFF Together / Ghidra headless binary analysis")
    line("This is decompiled pseudocode, NOT the original source.")
    line("File: " + currentProgram.getName())
    line("Architecture: " + str(currentProgram.getLanguageID()))
    line("Image base: " + str(currentProgram.getImageBase()))
    line("Function count: " + str(currentProgram.getFunctionManager().getFunctionCount()))
    decompiler = DecompInterface()
    decompiler.openProgram(currentProgram)
    targets = []
    if mode == "reference":
        # Addresses verified only for the supplied working 2026-11-09 DLL.
        for label, location in [("Fly Mode first reference", "180062ba0"),
                                ("Fly Mode second reference", "180063fc4")]:
            addr = toAddr(location)
            if currentProgram.getMemory().contains(addr):
                func = getFunctionContaining(addr)
                if func is not None:
                    targets.append((label, func))
                else:
                    line("No function at reference " + location)
            else:
                line("Reference address not mapped: " + location)
        # String references are leads, not proof of a runtime call path.
        for label, location in [("Fly Mode", "180cd5908"),
                                ("Fly Mode##2", "180cd5b40")]:
            addr = toAddr(location)
            if currentProgram.getMemory().contains(addr):
                line("String cross-references: " + label)
                for ref in getReferencesTo(addr):
                    line("  from " + str(ref.getFromAddress()))
    else:
        for f in currentProgram.getFunctionManager().getFunctions(True):
            name = f.getName().lower()
            if ("flymode" in name or "playercontroller" in name or "getplayerpawn" in name):
                targets.append((f.getName(), f))
                if len(targets) >= 4:
                    break
        if not targets:
            ep = currentProgram.getSymbolTable().getExternalEntryPointIterator()
            if ep.hasNext():
                func = getFunctionContaining(ep.next())
                if func is not None:
                    targets.append(("PE entry point", func))
        line("SMOKE mode: this verifies Ghidra decompilation on our own DLL, not the working reference.")
    seen = set()
    for label, func in targets[:5]:
        entry = str(func.getEntryPoint())
        if entry in seen:
            continue
        seen.add(entry)
        line("")
        line("TARGET: " + label + " at " + entry + " function " + func.getName())
        result = decompiler.decompileFunction(func, 90, monitor)
        if not result.decompileCompleted():
            line("Decompiler error: " + str(result.getErrorMessage()))
            continue
        result_function = result.getDecompiledFunction()
        if result_function is not None:
            code = result_function.getC()
            line(code[:50000])
            if len(code) > 50000:
                line("[pseudocode truncated at 50,000 chars]")
    if not targets:
        line("No matching functions found: static import still completed.")
    line("ANALYSIS_DONE")
finally:
    writer.close()
