// Shared Javascript for RB_Nexus Docs Website
document.addEventListener("DOMContentLoaded", () => {
  // Add Copy button to all pre blocks
  document.querySelectorAll("pre").forEach(pre => {
    const container = document.createElement("div");
    container.className = "code-container";
    pre.parentNode.insertBefore(container, pre);
    container.appendChild(pre);

    const btn = document.createElement("button");
    btn.className = "copy-btn";
    btn.innerText = "Copy";
    btn.addEventListener("click", () => {
      navigator.clipboard.writeText(pre.innerText).then(() => {
        btn.innerText = "Copied!";
        setTimeout(() => { btn.innerText = "Copy"; }, 2000);
      });
    });
    container.appendChild(btn);
  });
});
