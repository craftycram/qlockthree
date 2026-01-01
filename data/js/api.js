/* qlockthree API Utilities */

const API = {
  /**
   * GET request returning JSON
   */
  async get(url) {
    const response = await fetch(url);
    return response.json();
  },

  /**
   * GET request returning text
   */
  async getText(url) {
    const response = await fetch(url);
    return response.text();
  },

  /**
   * POST request with form data
   * @param {string} url - Endpoint URL
   * @param {Object|string} data - Data to send (object will be URL-encoded)
   */
  async post(url, data = '') {
    const body = typeof data === 'object'
      ? Object.entries(data).map(([k, v]) => `${k}=${encodeURIComponent(v)}`).join('&')
      : data;

    const response = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    return response.text();
  },

  /**
   * POST request returning JSON
   */
  async postJSON(url, data = '') {
    const body = typeof data === 'object'
      ? Object.entries(data).map(([k, v]) => `${k}=${encodeURIComponent(v)}`).join('&')
      : data;

    const response = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    return response.json();
  },

  /**
   * POST with confirmation dialog
   * @param {string} message - Confirmation message
   * @param {string} url - Endpoint URL
   * @param {Object|string} data - Data to send
   */
  async confirm(message, url, data = '') {
    if (confirm(message)) {
      return this.post(url, data);
    }
    return null;
  }
};
