/**
 * UniTrack – Shared Navigation Controller
 * Include this script at the bottom of every page's <body>.
 * It wires sidebar nav items to their pages and highlights the active one.
 */
(function () {
  const PAGES = [
    { label: 'Home',         file: 'home.html'         },
    { label: 'Schedule',     file: 'schedule.html'     },
    { label: 'Registration', file: 'registration.html' },
    { label: 'Alerts',       file: 'alerts.html'       },
  ];

  // Detect which page we're on from the filename
  const currentFile = window.location.pathname.split('/').pop() || 'home.html';

  // Wire up each nav item
  document.querySelectorAll('.nav-item').forEach((item) => {
    const label = item.textContent.trim().split('\n')[0].trim();
    const page  = PAGES.find(p => p.label === label);
    if (!page) return;

    // Highlight active page
    if (page.file === currentFile) {
      item.classList.add('active');
    } else {
      item.classList.remove('active');
    }

    // Navigate on click
    item.style.cursor = 'pointer';
    item.addEventListener('click', () => {
      if (page.file !== currentFile) {
        window.location.href = page.file;
      }
    });
  });
})();
