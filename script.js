document.addEventListener('DOMContentLoaded', () => {
    const navLinks = document.querySelectorAll('.nav-links a');
    const sections = document.querySelectorAll('section[id]');
    const abstractSection = document.querySelector('#abstract');
    const posterSection = document.querySelector('#poster');
    const sourceSection = document.querySelector('#source');
    const memberSection = document.querySelector('#member');
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const targetId = this.getAttribute('href');
            const targetSection = document.querySelector(targetId);
            if (!targetSection) return;

            targetSection.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });

            navLinks.forEach(l => l.classList.remove('active'));
            this.classList.add('active');
        });
    });

    if (sections.length) {
        const io = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                const id = entry.target.id;
                const link = document.querySelector(`.nav-links a[href="#${id}"]`);

                if (entry.isIntersecting) {
                    navLinks.forEach(l => l.classList.remove('active'));
                    if (link) link.classList.add('active');

                    // Add in-view class to visible sections
                    switch(id) {
                        case 'abstract':
                            if (abstractSection) abstractSection.classList.add('in-view');
                            break;
                        case 'poster':
                            if (posterSection) posterSection.classList.add('in-view');
                            break;
                        case 'source':
                            if (sourceSection) sourceSection.classList.add('in-view');
                            break;
                        case 'member':
                            if (memberSection) memberSection.classList.add('in-view');
                            break;
                    }
                } else {
                    switch(id) {
                        case 'abstract':
                            if (abstractSection) abstractSection.classList.remove('in-view');
                            break;
                        case 'poster':
                            if (posterSection) posterSection.classList.remove('in-view');
                            break;
                        case 'source':
                            if (sourceSection) sourceSection.classList.remove('in-view');
                            break;
                        case 'member':
                            if (memberSection) memberSection.classList.remove('in-view');
                            break;
                    }
                }
            });
        }, {
            root: null,
            rootMargin: '-20% 0px -55% 0px',
            threshold: 0
        });

        sections.forEach(section => io.observe(section));
    }
});