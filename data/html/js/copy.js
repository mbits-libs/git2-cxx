(() => {
	function sleep(ms) {
		return new Promise((resolve) => { setTimeout(resolve, ms); });
	}
	/**
	 *
	 * @param {HTMLElement} link
	 */
	async function copyPath(link) {
		if (link.getAttribute('data-copy') !== null) {
			return;
		}
		link.setAttribute('data-copy', '1');
		const path = link.getAttribute('data-path');

		try {
			await navigator.clipboard.writeText(path);
			link.classList.remove('ready');
			link.classList.add('ok');
		} catch (cause) {
			console.error(cause);
			link.classList.remove('ready');
			link.classList.add('failed');
		}

		await sleep(1000);
		link.classList.remove('ok');
		link.classList.remove('failed');
		link.classList.add('ready');
		link.removeAttribute('data-copy');
	}

	window.onload = () => {
		const links = Array.from(document.querySelectorAll('[data-path]'));
		console.log(links);
		for (const link of links) {
			link.onclick = () => copyPath(link);
			link.classList.remove('ok');
			link.classList.remove('failed');
			link.classList.add('ready');
		}
	};
})()
