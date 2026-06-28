import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";

export default function (pi: ExtensionAPI) {
	// Helper function to initialize Git if it doesn't exist
	async function ensureGitRepo(ctx: any) {
		const { code } = await pi.exec("git", ["rev-parse", "--is-inside-work-tree"]);
		if (code !== 0) {
			ctx.ui.notify("Initializing Git repository...", "info");
			await pi.exec("git", ["init"]);
			await pi.exec("git", ["add", "-A"]);
			await pi.exec("git", ["commit", "-m", "Initial commit (automatically created by Pi)"]);
			ctx.ui.notify("Git repository initialized and initial commit created.", "info");
		}
	}

	// Run ensureGitRepo on session startup
	pi.on("session_start", async (_event, ctx) => {
		await ensureGitRepo(ctx);
	});

	// Automatically stage and commit changes after the agent finishes processing a user prompt
	pi.on("agent_end", async (event, ctx) => {
		await ensureGitRepo(ctx);

		// Check if there are any changes (modified, deleted, untracked files)
		const { stdout: status, code } = await pi.exec("git", ["status", "--porcelain"]);

		if (code !== 0 || status.trim().length === 0) {
			// Not a git repo or no changes to commit
			return;
		}

		// Find the user's last message to use as context for the commit message
		const entries = ctx.sessionManager.getEntries();
		let lastUserText = "";
		for (let i = entries.length - 1; i >= 0; i--) {
			const entry = entries[i];
			if (entry.type === "message" && entry.message.role === "user") {
				const content = entry.message.content;
				if (typeof content === "string") {
					lastUserText = content;
				} else if (Array.isArray(content)) {
					lastUserText = content
						.filter((c): c is { type: "text"; text: string } => c.type === "text")
						.map((c) => c.text)
						.join("\n");
				}
				break;
			}
		}

		// Generate a clean commit message from the user's prompt
		const cleanText = lastUserText.replace(/\r?\n|\r/g, " ").trim();
		const commitMessage = cleanText
			? `[pi] Auto-commit: ${cleanText.slice(0, 60)}${cleanText.length > 60 ? "..." : ""}`
			: "[pi] Auto-commit after prompt";

		// Stage all changes
		await pi.exec("git", ["add", "-A"]);
		
		// Commit
		const { code: commitCode, stderr } = await pi.exec("git", ["commit", "-m", commitMessage]);

		if (commitCode === 0 && ctx.hasUI) {
			ctx.ui.notify(`Auto-committed: ${commitMessage}`, "info");
		} else if (commitCode !== 0) {
			ctx.ui.notify(`Auto-commit failed: ${stderr.trim()}`, "error");
		}
	});
}
