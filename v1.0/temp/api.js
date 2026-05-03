/**
 * UniTrack – API Connector
 * Connects the HTML frontend to the C++ httplib backend at localhost:8080.
 * Include this script on every page: <script src="api.js"></script>
 *
 * On success: returns the parsed JSON body.
 * On error:   returns an object { _error: true, status, message }
 *             so callers can surface the server's message to the user
 *             (e.g. "Time conflict with CMPS1044").
 *
 * Code that previously checked `if (result)` should now use
 * `if (result && !result._error)` to distinguish success from failure.
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

// Parse the response body. If the request failed, build an error object
// that includes the server's "error" message so the UI can show it.
async function _handleResponse(res, path, method) {
  let body = null;
  try { body = await res.json(); } catch (e) { /* not JSON */ }

  if (!res.ok) {
    const message = (body && body.error) ? body.error
                                         : `${method} ${path} failed: ${res.status}`;
    return { _error: true, status: res.status, message };
  }
  return body;
}

async function apiPost(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    return await _handleResponse(res, path, 'POST');
  } catch (err) {
    console.error(err);
    return { _error: true, status: 0, message: 'Network error — is the server running?' };
  }
}

async function apiPut(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    return await _handleResponse(res, path, 'PUT');
  } catch (err) {
    console.error(err);
    return { _error: true, status: 0, message: 'Network error — is the server running?' };
  }
}

async function apiDelete(path, body) {
  try {
    const res = await fetch(API_BASE + path, {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    return await _handleResponse(res, path, 'DELETE');
  } catch (err) {
    console.error(err);
    return { _error: true, status: 0, message: 'Network error — is the server running?' };
  }
}

// ── API calls — one function per endpoint ─────────────────────────────

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

  // POST /alerts/dismiss — hide an alert by fingerprint
  dismissAlert: (type, courseCode, message) =>
    apiPost('/alerts/dismiss', { type, courseCode, message }),

  // POST /alerts/restore — un-dismiss everything
  restoreAlerts: () => apiPost('/alerts/restore', {}),

  // POST /save — persist student data to file
  save: () => apiPost('/save', {}),
};