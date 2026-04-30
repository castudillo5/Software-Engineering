/**
 * UniTrack – API Connector
 * Connects the HTML frontend to the C++ httplib backend at localhost:8080.
 * Include this script on every page: <script src="api.js"></script>
 * All functions are async and return parsed JSON or null on error.
 */

const API_BASE = 'http://localhost:8080';

// ── Generic fetch helpers ──────────────────────────────────────────────

async function apiGet(path) {
  try {
    const res = await fetch(API_BASE + path);
    if (!res.ok) throw new Error(`GET ${path} failed: ${res.status}`);
    return await res.json();
  } catch (err) {
    console.error(err);
    return null;
  }
}

async function apiPost(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    if (!res.ok) throw new Error(`POST ${path} failed: ${res.status}`);
    return await res.json();
  } catch (err) {
    console.error(err);
    return null;
  }
}

async function apiPut(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    if (!res.ok) throw new Error(`PUT ${path} failed: ${res.status}`);
    return await res.json();
  } catch (err) {
    console.error(err);
    return null;
  }
}

async function apiDelete(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    if (!res.ok) throw new Error(`DELETE ${path} failed: ${res.status}`);
    return await res.json();
  } catch (err) {
    console.error(err);
    return null;
  }
}

// ── API calls — one function per endpoint ─────────────────────────────

// Check if a student has been set up yet
const API = {

  status: () => apiGet('/status'),

  // GET /student — full student object
  getStudent: () => apiGet('/student'),

  // POST /student — first-time setup
  createStudent: (name, major) => apiPost('/student', { name, major }),

  // PUT /student — update name or major
  updateStudent: (name, major) => apiPut('/student', { name, major }),

  // GET /courses — full course catalog
  getCourses: () => apiGet('/courses'),

  // GET /audit — degree audit with requirement categories
  getAudit: () => apiGet('/audit'),

  // GET /alerts — active alerts list
  getAlerts: () => apiGet('/alerts'),

  // POST /record — add a course to academic history
  addRecord: (courseCode, grade, semester, completed) =>
    apiPost('/record', { courseCode, grade, semester, completed: String(completed) }),

  // PUT /record — update a grade
  updateGrade: (courseCode, grade) =>
    apiPut('/record', { courseCode, grade }),

  // DELETE /record — remove a course record
  deleteRecord: (courseCode) =>
    apiDelete('/record', { courseCode }),

  // POST /flex — assign course to a gen-ed flex category
  assignFlex: (courseCode, categoryCode) =>
    apiPost('/flex', { courseCode, categoryCode }),

  // POST /save — persist student data to file
  save: () => apiPost('/save', {}),
};
