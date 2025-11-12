import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import subprocess
import os
import threading

# Path to your compiled C++ executable
EXECUTABLE_PATH = os.path.join("build", "Release", "sp_dedup.exe")

# --- Main function that runs deduplication ---
def run_dedup():
    folder = filedialog.askdirectory(title="Select folder to scan")
    if not folder:
        return

    # Clear old output
    output_box.delete(1.0, tk.END)

    within = var_within.get()
    across = var_across.get()
    commit = var_commit.get()

    # Build command
    cmd = [EXECUTABLE_PATH, folder, "--recurse", "--only-ext=.docx,.xlsx,.txt"]

    if within:
        cmd.append("--within")
    if commit:
        cmd.append("--commit")

    # If neither selected, warn
    if not within and not across:
        messagebox.showwarning("No option selected",
                               "Please select at least one operation (file or data deduplication).")
        return

    output_box.insert(tk.END, f"▶ Running command:\n{' '.join(cmd)}\n\n")

    # Run in background thread to keep UI responsive
    def task():
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            output_box.insert(tk.END, result.stdout)
            if result.stderr:
                output_box.insert(tk.END, "\nErrors:\n" + result.stderr)
        except Exception as e:
            output_box.insert(tk.END, f"Error: {e}\n")

    threading.Thread(target=task, daemon=True).start()


# --- UI Setup ---
root = tk.Tk()
root.title("File Deduplication Tool (SP Project)")
root.geometry("700x500")
root.resizable(False, False)

# Header
tk.Label(root, text="📁 File Encryption, Decryption & Deduplication Project",
         font=("Segoe UI", 12, "bold"), pady=10).pack()

# Checkboxes for operations
frame = tk.Frame(root)
frame.pack(pady=5)

var_across = tk.BooleanVar(value=True)
var_within = tk.BooleanVar(value=True)
var_commit = tk.BooleanVar(value=False)

tk.Checkbutton(frame, text="Remove duplicate files (across files)", variable=var_across).grid(row=0, column=0, sticky="w", padx=10)
tk.Checkbutton(frame, text="Remove duplicate data (within files)", variable=var_within).grid(row=1, column=0, sticky="w", padx=10)
tk.Checkbutton(frame, text="Apply changes (--commit)", variable=var_commit).grid(row=2, column=0, sticky="w", padx=10)

# Run button
tk.Button(root, text="Select Folder & Run", command=run_dedup, bg="#1976D2", fg="white",
          font=("Segoe UI", 10, "bold"), padx=20, pady=5).pack(pady=10)

# Output box
output_box = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=80, height=20)
output_box.pack(padx=10, pady=5)

# Footer
tk.Label(root, text="Developed for System Programming Course – Phase 3 GUI Frontend",
         font=("Segoe UI", 9), fg="gray").pack(side="bottom", pady=5)

root.mainloop()
