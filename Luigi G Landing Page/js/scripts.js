document.addEventListener("DOMContentLoaded", function() {
    // Seleciona o formulário
    const form = document.getElementById("contactForm");
  
    // Adiciona um ouvinte de evento para o envio do formulário
    form.addEventListener("submit", function(event) {
        // Previne o envio padrão do formulário
        event.preventDefault();
  
        // Obtém os dados do formulário
        const formData = {
            nome: document.getElementById("name").value,
            email: document.getElementById("email").value,
            telefone: document.getElementById("phone").value,
            mensagem: document.getElementById("message").value
        };
  
        // Exibe os dados do formulário no console em formato JSON
        console.log("Dados:", formData);
  
        // Simula uma requisição POST
        console.log("Enviando requisição...");
  
        // Simulação de uma requisição POST utilizando fetch
        fetch("http://localhost:9999/enviodados", {  // Corrigida a URL
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(formData)
        })
        .then(response => {
            console.log('Response received:', response);
            return response.json();
        })
        .then(data => {
            console.log("Sucesso:", data);
            alert("Dados enviados com sucesso!");
        })
        .catch(error => {
            console.error("Erro:", error);
            alert("Ocorreu um erro ao enviar os dados.");
        });
  
        // Limpa os campos do formulário (opcional)
        form.reset();
    });
  });