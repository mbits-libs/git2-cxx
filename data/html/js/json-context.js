(() => {
	window.onload = () => {
		console.log(json);
		console.log(json.lines);
		const div = document.createElement('div');
		div.classList.add('card');
		div.style.padding = '1rem';
		document.querySelector('main').append(div);
		Object.entries(json.octicons).forEach(([key, def]) => {
			const span = document.createElement('span');
			span.setAttribute('title', key);
			span.innerHTML = def;
			div.append(span)
			div.append(' ');
		});
	};
})()
